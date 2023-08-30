#include <vector>

#include <cds/gc/details/hp_smr_common.h>
#include <cds/gc/details/hp_scan_functions.h>

namespace
{
    struct defaults {
        static const size_t c_nHazardPointerPerThread = 8;
        static const size_t c_nMaxThreadCount = 100;
    };

    size_t calc_retired_size( size_t nSize, size_t nHPCount, size_t nThreadCount )
    {
        size_t const min_size = nHPCount * nThreadCount;
        return nSize < min_size ? min_size * 2 : nSize;
    }
}

namespace cds { namespace gc { namespace hp { namespace details {

    CDS_EXPORT_API smr_common_data::smr_common_data(size_t nHazardPtrCount, size_t nMaxThreadCount, size_t nMaxRetiredPtrCount, scan_type nScanType )
        : hazard_ptr_count_( nHazardPtrCount == 0 ? defaults::c_nHazardPointerPerThread : nHazardPtrCount )
        , max_thread_count_( nMaxThreadCount == 0 ? defaults::c_nMaxThreadCount : nMaxThreadCount )
        , max_retired_ptr_count_( calc_retired_size( nMaxRetiredPtrCount, hazard_ptr_count_, max_thread_count_ ))
        , scan_type_( nScanType )
    {
        thread_list_.store( nullptr, atomics::memory_order_release );
    }

    CDS_EXPORT_API client_record* create_client_data(size_t hazard_ptr_count, size_t max_retired_ptr_count, s_alloc_memory_t s_alloc_memory)
    {
        size_t const guard_array_size = thread_hp_storage::calc_array_size(hazard_ptr_count);
        size_t const retired_array_size = retired_array::calc_array_size(max_retired_ptr_count);
        size_t const nSize = sizeof( client_record ) + guard_array_size + retired_array_size;

        /*
            The memory is allocated by contnuous block
            Memory layout:
            +--------------------------+
            |                          |
            | thread_record            |
            |         hazards_         +---+
        +---|         retired_         |   |
        |   |                          |   |
        |   |--------------------------|   |
        |   | hazard_ptr[]             |<--+
        |   |                          |
        |   |                          |
        |   |--------------------------|
        +-->| retired_ptr[]            |
            |                          |
            |                          |
            +--------------------------+
        */

        uint8_t* mem = reinterpret_cast<uint8_t*>( s_alloc_memory( nSize ));

        return new( mem ) client_record(
            reinterpret_cast<guard*>( mem + sizeof( client_record )),
            hazard_ptr_count,
            reinterpret_cast<retired_ptr*>( mem + sizeof( client_record ) + guard_array_size ),
            max_retired_ptr_count
        );
    }

    CDS_EXPORT_API client_record* alloc_thread_data(
        atomics::atomic<client_record *> &thread_list,
        size_t hazard_ptr_count, size_t max_retired_ptr_count, s_alloc_memory_t s_alloc_memory)
    {
        client_record *hprec;

        // First try to reuse a free (non-active) HP record
        for (hprec = thread_list.load(atomics::memory_order_acquire); hprec; hprec = hprec->next_)
        {
            client_record *null_rec = nullptr;
            if (!hprec->owner_rec_.compare_exchange_strong(null_rec, hprec, atomics::memory_order_relaxed, atomics::memory_order_relaxed)) {
                continue;
            }
            hprec->free_.store(false, atomics::memory_order_release);
            return hprec;
        }

        // No HP records available for reuse
        // Allocate and push a new HP record
        hprec = create_client_data(hazard_ptr_count, max_retired_ptr_count, s_alloc_memory);
        hprec->owner_rec_.store(hprec, atomics::memory_order_relaxed);

        client_record *pOldHead = thread_list.load(atomics::memory_order_relaxed);
        do
        {
            hprec->next_ = pOldHead;
        } while (!thread_list.compare_exchange_weak(pOldHead, hprec, atomics::memory_order_release, atomics::memory_order_acquire));

        return hprec;
    }
}}}} // namespace cds::gc::hp::details