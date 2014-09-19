//$$CDS-header$$

#ifndef _CDS_URCU_GENERAL_INSTANT_H
#define _CDS_URCU_GENERAL_INSTANT_H

#include <cds/urcu/details/gpi.h>

namespace cds { namespace urcu {

    /// User-space general-purpose RCU with immediate reclamation
    /** @anchor cds_urcu_general_instant_gc

        This is a wrapper around general_instant class used for metaprogramming.

        Template arguments:
        - \p Lock - mutex type, default is \p std::mutex
        - \p Backoff - back-off schema, default is cds::backoff::Default
    */
    template <
#ifdef CDS_DOXGEN_INVOKED
        class Lock = std::mutex
       ,class Backoff = cds::backoff::Default
#else
        class Lock
       ,class Backoff
#endif
    >
    class gc< general_instant< Lock, Backoff > >: public details::gc_common
    {
    public:
        typedef general_instant< Lock, Backoff >        rcu_implementation   ;   ///< Wrapped URCU implementation

        typedef typename rcu_implementation::rcu_tag     rcu_tag     ;   ///< URCU tag
        typedef typename rcu_implementation::thread_gc   thread_gc   ;   ///< Thread-side RCU part
        typedef typename rcu_implementation::scoped_lock scoped_lock ;   ///< Access lock class

        using details::gc_common::atomic_marked_ptr;

    public:
        /// Creates URCU \p %general_instant singleton
        gc()
        {
            rcu_implementation::Construct();
        }

        /// Destroys URCU \p %general_instant singleton
        ~gc()
        {
            rcu_implementation::Destruct( true );
        }

    public:
        /// Waits to finish a grace period
        static void synchronize()
        {
            rcu_implementation::instance()->synchronize();
        }

        /// Frees the pointer \p p invoking \p pFunc after end of grace period
        /**
            The function calls \ref synchronize to wait for end of grace period
            and then evaluates disposing expression <tt>pFunc( p )</tt>
        */
        template <typename T>
        static void retire_ptr( T * p, void (* pFunc)(T *) )
        {
            retired_ptr rp( reinterpret_cast<void *>( p ), reinterpret_cast<free_retired_ptr_func>( pFunc ) );
            retire_ptr( rp );
        }

        /// Frees the pointer \p using \p Disposer after end of grace period
        /**
            The function calls \ref synchronize to wait for end of grace period
            and then evaluates disposing expression <tt>Disposer()( p )</tt>
        */
        template <typename Disposer, typename T>
        static void retire_ptr( T * p )
        {
            retire_ptr( p, cds::details::static_functor<Disposer, T>::call );
        }

        /// Frees the pointer \p after the end of grace period
        /**
            The function calls \ref synchronize to wait for end of grace period
            and then evaluates disposing expression <tt>p.m_funcFree( p.m_p )</tt>
        */
        static void retire_ptr( retired_ptr& p )
        {
            rcu_implementation::instance()->retire_ptr(p);
        }

        /// Frees chain [ \p itFirst, \p itLast) in one synchronization cycle
        template <typename ForwardIterator>
        static void batch_retire( ForwardIterator itFirst, ForwardIterator itLast )
        {
            rcu_implementation::instance()->batch_retire( itFirst, itLast );
        }

        /// Acquires access lock (so called RCU reader-side lock)
        /**
            For safety reasons, it is better to use \ref scoped_lock class for locking/unlocking
        */
        static void access_lock()
        {
            thread_gc::access_lock();
        }

        /// Releases access lock (so called RCU reader-side lock)
        /**
            For safety reasons, it is better to use \ref scoped_lock class for locking/unlocking
        */
        static void access_unlock()
        {
            thread_gc::access_unlock();
        }

        /// Checks if the thread is inside read-side critical section (i.e. the lock is acquired)
        /**
            Usually, this function is used internally to be convinced
            that subsequent remove action is not lead to a deadlock.
        */
        static bool is_locked()
        {
            return thread_gc::is_locked();
        }

        /// Forced GC cycle call.
        /**
            This method does nothing and is introduced only for uniformity with other
            garbage collectors.
        */
        static void force_dispose()
        {}
    };

}} // namespace cds::urcu

#endif // #ifndef _CDS_URCU_GENERAL_INSTANT_H
