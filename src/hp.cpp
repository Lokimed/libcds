// Copyright (c) 2006-2018 Maxim Khizhinsky
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <algorithm>
#include <vector>

#include <cds/gc/hp.h>
#include <cds/gc/details/hp_scan_functions.h>
#include <cds/os/thread.h>
#include <cds/gc/hp_membar.h>
#include <cds/gc/details/hp_default_memory_allocator.h>

#if CDS_OS_TYPE == CDS_OS_LINUX
#   include <unistd.h>
#   include <sys/syscall.h>

    // membarrier() was added in Linux 4.3
#   if !defined( __NR_membarrier )
#       define __NR_membarrier 324
#   endif

#   ifdef CDS_HAVE_LINUX_MEMBARRIER_H
#       include <linux/membarrier.h>
#   else
#       define MEMBARRIER_CMD_QUERY                         0
#       define MEMBARRIER_CMD_SHARED                        (1<<0)
#   endif
    // linux 4.14+
#   define CDS_MEMBARRIER_CMD_PRIVATE_EXPEDITED             (1<<3)
#   define CDS_MEMBARRIER_CMD_REGISTER_PRIVATE_EXPEDITED    (1<<4)
#endif

namespace cds { namespace gc { namespace hp { namespace details {

    std::atomic<unsigned> shared_var_membar::shared_var_{ 0 };

#if CDS_OS_TYPE == CDS_OS_LINUX

    bool asymmetric_membar::membarrier_available_ = false;

    void asymmetric_membar::check_membarrier_available()
    {
        int res = syscall( __NR_membarrier, MEMBARRIER_CMD_QUERY, 0 );
        membarrier_available_ = !( res == -1 || ( res & CDS_MEMBARRIER_CMD_PRIVATE_EXPEDITED ) == 0 )
            && syscall( __NR_membarrier, CDS_MEMBARRIER_CMD_REGISTER_PRIVATE_EXPEDITED, 0 ) == 0;
    }

    void asymmetric_membar::call_membarrier()
    {
        assert( membarrier_available_ );
        syscall( __NR_membarrier, CDS_MEMBARRIER_CMD_PRIVATE_EXPEDITED, 0 );
    }

    bool asymmetric_global_membar::membarrier_available_ = false;

    void asymmetric_global_membar::check_membarrier_available()
    {
        int res = syscall( __NR_membarrier, MEMBARRIER_CMD_QUERY, 0 );
        membarrier_available_ = !( res == -1 || ( res & MEMBARRIER_CMD_SHARED ) == 0 );
    }

    void asymmetric_global_membar::call_membarrier()
    {
        assert( membarrier_available_ );
        syscall( __NR_membarrier, MEMBARRIER_CMD_SHARED, 0 );
    }

#endif

    namespace {

        void* ( *s_alloc_memory )( size_t size ) = default_alloc_memory;
        void ( *s_free_memory )( void* p ) = default_free_memory;

        template <typename T>
        class allocator
        {
        public:
            typedef T   value_type;

            allocator() {}
            allocator( allocator const& ) {}
            template <class U>
            explicit allocator( allocator<U> const& ) {}

            static T* allocate( size_t nCount )
            {
                return reinterpret_cast<T*>( s_alloc_memory( sizeof( value_type ) * nCount ));
            }

            static void deallocate( T* p, size_t /*nCount*/ )
            {
                s_free_memory( reinterpret_cast<void*>( p ));
            }
        };

        // TODO delete this in future
        struct defaults {
            static const size_t c_nHazardPointerPerThread = 8;
            static const size_t c_nMaxThreadCount = 100;
        };

        size_t calc_retired_size( size_t nSize, size_t nHPCount, size_t nThreadCount )
        {
            size_t const min_size = nHPCount * nThreadCount;
            return nSize < min_size ? min_size * 2 : nSize;
        }

        stat s_postmortem_stat;
    } // namespace

    /*static*/ CDS_EXPORT_API basic_smr* basic_smr::instance_ = nullptr;

    /*static*/ CDS_EXPORT_API void basic_smr::set_memory_allocator(
        void* ( *alloc_func )( size_t size ),
        void( *free_func )( void * p )
    )
    {
        // The memory allocation functions may be set BEFORE initializing HP SMR!!!
        assert( instance_ == nullptr );

        s_alloc_memory = alloc_func;
        s_free_memory = free_func;
    }


    /*static*/ CDS_EXPORT_API void basic_smr::construct(size_t nHazardPtrCount, size_t nMaxThreadCount, size_t nMaxRetiredPtrCount, scan_type nScanType )
    {
        if ( !instance_ ) {
            instance_ = new( s_alloc_memory(sizeof(basic_smr))) basic_smr(nHazardPtrCount, nMaxThreadCount, nMaxRetiredPtrCount, nScanType );
        }
    }

    /*static*/ CDS_EXPORT_API void basic_smr::destruct(bool bDetachAll )
    {
        if ( instance_ ) {
            if ( bDetachAll )
                instance_->detach_all_thread();

            instance_->~basic_smr();
            s_free_memory( instance_ );
            instance_ = nullptr;
        }
    }

    CDS_EXPORT_API basic_smr::basic_smr(size_t nHazardPtrCount, size_t nMaxThreadCount, size_t nMaxRetiredPtrCount, scan_type nScanType )
        : hazard_ptr_count_( nHazardPtrCount == 0 ? defaults::c_nHazardPointerPerThread : nHazardPtrCount )
        , max_thread_count_( nMaxThreadCount == 0 ? defaults::c_nMaxThreadCount : nMaxThreadCount )
        , max_retired_ptr_count_( calc_retired_size( nMaxRetiredPtrCount, hazard_ptr_count_, max_thread_count_ ))
        , scan_type_( nScanType )
        , scan_func_( nScanType == scan_type::classic ? &basic_smr::classic_scan : &basic_smr::inplace_scan )
    {
        thread_list_.store( nullptr, atomics::memory_order_release );
    }

    CDS_EXPORT_API basic_smr::~basic_smr()
    {
        CDS_HPSTAT( statistics( s_postmortem_stat ));

        thread_record* pHead = thread_list_.load( atomics::memory_order_relaxed );
        thread_list_.store( nullptr, atomics::memory_order_release );

        thread_record* pNext = nullptr;
        for ( thread_record* hprec = pHead; hprec; hprec = pNext )
        {
            assert( hprec->owner_rec_.load( atomics::memory_order_relaxed ) == nullptr
                || hprec->owner_rec_.load( atomics::memory_order_relaxed ) == hprec );

            retired_array& arr = hprec->retired_;
            for ( retired_ptr* cur{ arr.first() }, *last{ arr.last() }; cur != last; ++cur ) {
                cur->free();
                CDS_HPSTAT( ++s_postmortem_stat.free_count );
            }

            arr.reset( 0 );
            pNext = hprec->next_;
            hprec->free_.store( true, atomics::memory_order_relaxed );
            destroy_thread_data( hprec );
        }
    }


    CDS_EXPORT_API basic_smr::thread_record* basic_smr::create_thread_data()
    {
        return details::create_client_data(get_hazard_ptr_count(), get_max_retired_ptr_count(), s_alloc_memory);
    }

    /*static*/ CDS_EXPORT_API void basic_smr::destroy_thread_data(thread_record* pRec )
    {
        // all retired pointers must be freed
        assert( pRec->retired_.size() == 0 );

        pRec->~thread_record();
        s_free_memory( pRec );
    }


    CDS_EXPORT_API basic_smr::thread_record* basic_smr::alloc_thread_data()
    {
        return details::alloc_thread_data(thread_list_, get_hazard_ptr_count(), get_max_retired_ptr_count(), s_alloc_memory);
    }

    CDS_EXPORT_API void basic_smr::free_thread_data(basic_smr::thread_record* pRec, bool callHelpScan )
    {
        assert( pRec != nullptr );

        pRec->hazards_.clear();
        scan( pRec );
        if ( callHelpScan )
            help_scan( pRec );
        pRec->owner_rec_.store( nullptr, atomics::memory_order_release );
    }

    CDS_EXPORT_API void basic_smr::detach_all_thread()
    {
        thread_record * pNext = nullptr;

        for ( thread_record * hprec = thread_list_.load( atomics::memory_order_relaxed ); hprec; hprec = pNext ) {
            pNext = hprec->next_;
            if ( hprec->owner_rec_.load( atomics::memory_order_relaxed ) != nullptr ) {
                free_thread_data( hprec, false );
            }
        }
    }

    namespace
    {
        using VectorVoid = std::vector< void*, allocator<void*>>;

        VectorVoid createVectorWithCustomAllocator(size_t thread_count, size_t hazard_ptr_count)
        {
            VectorVoid plist;
            plist.reserve( thread_count * hazard_ptr_count);
            return plist;    
        }    
    }
    

    CDS_EXPORT_API void basic_smr::classic_scan(thread_data *pRec)
    {
        details::HpMechanizmPart hp_data{pRec, thread_list_};
        auto thread_count = get_max_thread_count();
        auto hazard_ptr_count = get_hazard_ptr_count();
        
        auto vector_creator = [thread_count, hazard_ptr_count]() -> VectorVoid
        {
            return createVectorWithCustomAllocator(thread_count, hazard_ptr_count);
        };
        
        details::classic_scan<VectorVoid>(hp_data, std::move(vector_creator));
    }

    CDS_EXPORT_API void basic_smr::help_scan(thread_data *pThis)
    {
        assert(static_cast<thread_record *>(pThis)->owner_rec_.load(atomics::memory_order_relaxed) == static_cast<thread_record *>(pThis));

        CDS_HPSTAT(++pThis->help_scan_count_);

        for (thread_record *hprec = thread_list_.load(atomics::memory_order_acquire); hprec; hprec = hprec->next_)
        {
            if (hprec == static_cast<thread_record *>(pThis))
                continue;

            // If free_ == true then hprec->retired_ is empty - we don't need to see it
            if (hprec->free_.load(atomics::memory_order_acquire))
                continue;

            // Owns hprec if it is empty.
            // Several threads may work concurrently so we use atomic technique only.
            {
                thread_record *curOwner = hprec->owner_rec_.load(atomics::memory_order_relaxed);
                if (curOwner == nullptr)
                {
                    if (!hprec->owner_rec_.compare_exchange_strong(curOwner, hprec, atomics::memory_order_acquire, atomics::memory_order_relaxed))
                        continue;
                }
                else
                    continue;
            }

            // We own the thread record successfully. Now, we can see whether it has retired pointers.
            // If it has ones then we move them to pThis that is private for current thread.
            retired_array &src = hprec->retired_;
            retired_array &dest = pThis->retired_;
            assert(!dest.full());

            retired_ptr *src_first = src.first();
            retired_ptr *src_last = src.last();

            for (; src_first != src_last; ++src_first)
            {
                if (!dest.push(std::move(*src_first)))
                    scan(pThis);
            }

            src.interthread_clear();
            hprec->free_.store(true, atomics::memory_order_release);
            hprec->owner_rec_.store(nullptr, atomics::memory_order_release);

            scan(pThis);
        }
    }

    CDS_EXPORT_API void basic_smr::inplace_scan(thread_data* pThreadRec )
    {
        details::HpMechanizmPart hp_data{pThreadRec, thread_list_};
        auto thread_count = get_max_thread_count();
        auto hazard_ptr_count = get_hazard_ptr_count();

        auto vector_creator = [thread_count, hazard_ptr_count](void) -> VectorVoid
        {
            return createVectorWithCustomAllocator(thread_count, hazard_ptr_count);
        };
        
        details::inplace_scan<VectorVoid>(hp_data, std::move(vector_creator));
    }

    CDS_EXPORT_API void basic_smr::statistics(stat& st )
    {
        st.clear();
#   ifdef CDS_ENABLE_HPSTAT
        for ( thread_record* hprec = thread_list_.load( atomics::memory_order_acquire ); hprec; hprec = hprec->next_ )
        {
            CDS_TSAN_ANNOTATE_IGNORE_READS_BEGIN;
            ++st.thread_rec_count;
            st.guard_allocated += hprec->hazards_.alloc_guard_count_;
            st.guard_freed     += hprec->hazards_.free_guard_count_;
            st.retired_count   += hprec->retired_.retire_call_count_;
            st.free_count      += hprec->free_count_;
            st.scan_count      += hprec->scan_count_;
            st.help_scan_count += hprec->help_scan_count_;
            CDS_TSAN_ANNOTATE_IGNORE_READS_END;
        }
#   endif
    }

    cds::gc::hp::details::stat const& postmortem_statistics()
    {
        return s_postmortem_stat;
    }

}}}} // namespace cds::gc::hp::details
