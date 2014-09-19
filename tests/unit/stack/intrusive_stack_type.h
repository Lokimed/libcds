//$$CDS-header$$

#ifndef __CDSUNIT_INTRUSIVE_STACK_TYPES_H
#define __CDSUNIT_INTRUSIVE_STACK_TYPES_H

#include <cds/intrusive/treiber_stack.h>
#include <cds/intrusive/michael_deque.h>
#include <cds/intrusive/fcstack.h>

#include <cds/gc/hp.h>
#include <cds/gc/ptb.h>
#include <cds/gc/hrc.h>

#include <mutex>
#include <cds/lock/spinlock.h>
#include <stack>
#include <list>
#include <vector>
#include <boost/intrusive/list.hpp>

namespace istack {

    namespace details {
        template <typename GC, typename T, CDS_DECL_OPTIONS10>
        class MichaelDequeL: public cds::intrusive::MichaelDeque< GC, T, CDS_OPTIONS10>
        {
            typedef cds::intrusive::MichaelDeque< GC, T, CDS_OPTIONS10> base_class;
        public:
            MichaelDequeL( size_t nMaxItemCount )
                : base_class( (unsigned int) nMaxItemCount, 4 )
                {}

            bool push( T& v )
            {
                return base_class::push_front( v );
            }

            T * pop()
            {
                return base_class::pop_front();
            }
        };

        template <typename GC, typename T, CDS_DECL_OPTIONS10>
        class MichaelDequeR: public cds::intrusive::MichaelDeque< GC, T, CDS_OPTIONS10>
        {
            typedef cds::intrusive::MichaelDeque< GC, T, CDS_OPTIONS10> base_class;
        public:
            MichaelDequeR( size_t nMaxItemCount )
                : base_class( (unsigned int) nMaxItemCount, 4 )
            {}

            bool push( T& v )
            {
                return base_class::push_back( v );
            }

            T * pop()
            {
                return base_class::pop_back();
            }
        };

        template < typename T, typename Stack, typename Lock>
        class StdStack
        {
            Stack   m_Impl;
            mutable Lock    m_Lock;
            cds::intrusive::treiber_stack::empty_stat m_stat;

            typedef std::unique_lock<Lock>  unique_lock;

        public:
            typedef T value_type;

            bool push( T& v )
            {
                unique_lock l( m_Lock );
                m_Impl.push( &v );
                return true;
            }

            T * pop()
            {
                unique_lock l( m_Lock );
                if ( !m_Impl.empty() ) {
                     T * v = m_Impl.top();
                    m_Impl.pop();
                    return v;
                }
                return NULL;
            }

            bool empty() const
            {
                unique_lock l( m_Lock );
                return m_Impl.empty();
            }

            cds::intrusive::treiber_stack::empty_stat const& statistics() const
            {
                return m_stat;
            }
        };
    }

    template <typename T>
    struct Types {

    // TreiberStack
        typedef cds::intrusive::TreiberStack< cds::gc::HP, T
        >       Treiber_HP;

        typedef cds::intrusive::TreiberStack< cds::gc::HP, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::HP > > >
            ,cds::opt::memory_model<cds::opt::v::sequential_consistent>
        >       Treiber_HP_seqcst;

        typedef cds::intrusive::TreiberStack< cds::gc::HP, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::HP > > >
            ,cds::opt::stat<cds::intrusive::treiber_stack::stat<> >
        >       Treiber_HP_stat;

        typedef cds::intrusive::TreiberStack< cds::gc::HRC, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::HRC > > >
        >       Treiber_HRC;

        typedef cds::intrusive::TreiberStack< cds::gc::HRC, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::HRC > > >
            ,cds::opt::stat<cds::intrusive::treiber_stack::stat<> >
        >       Treiber_HRC_stat;

        typedef cds::intrusive::TreiberStack< cds::gc::PTB, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::PTB > > >
        > Treiber_PTB;

        typedef cds::intrusive::TreiberStack< cds::gc::PTB, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::PTB > > >
            ,cds::opt::stat<cds::intrusive::treiber_stack::stat<> >
        >       Treiber_PTB_stat;

        typedef cds::intrusive::TreiberStack< cds::gc::HP, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::HP > > >
            ,cds::opt::back_off<cds::backoff::yield>
            ,cds::opt::memory_model<cds::opt::v::relaxed_ordering>
        > Treiber_HP_yield;

        typedef cds::intrusive::TreiberStack< cds::gc::HP, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::HP > > >
            ,cds::opt::back_off<cds::backoff::pause>
        > Treiber_HP_pause;

        typedef cds::intrusive::TreiberStack< cds::gc::HP, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::HP > > >
            ,cds::opt::back_off<
                cds::backoff::exponential<
                    cds::backoff::pause,
                    cds::backoff::yield
                >
            >
        > Treiber_HP_exp;

        typedef cds::intrusive::TreiberStack< cds::gc::HRC, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::HRC > > >
            ,cds::opt::back_off<cds::backoff::yield>
        >  Treiber_HRC_yield;

        typedef cds::intrusive::TreiberStack< cds::gc::HRC, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::HRC > > >
            ,cds::opt::back_off<cds::backoff::pause>
            ,cds::opt::memory_model<cds::opt::v::relaxed_ordering>
        > Treiber_HRC_pause;

        typedef cds::intrusive::TreiberStack< cds::gc::HRC, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::HRC > > >
            ,cds::opt::back_off<
                cds::backoff::exponential<
                    cds::backoff::pause,
                    cds::backoff::yield
                >
            >
        > Treiber_HRC_exp;

        typedef cds::intrusive::TreiberStack< cds::gc::PTB, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::PTB > > >
            ,cds::opt::back_off<cds::backoff::yield>
        >  Treiber_PTB_yield;

        typedef cds::intrusive::TreiberStack< cds::gc::PTB, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::PTB > > >
            ,cds::opt::back_off<cds::backoff::pause>
        > Treiber_PTB_pause;

        typedef cds::intrusive::TreiberStack< cds::gc::PTB, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::PTB > > >
            ,cds::opt::back_off<
                cds::backoff::exponential<
                    cds::backoff::pause,
                    cds::backoff::yield
                >
            >
            ,cds::opt::memory_model<cds::opt::v::relaxed_ordering>
        > Treiber_PTB_exp;


    // Elimination stack
        typedef cds::intrusive::TreiberStack< cds::gc::HP, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::HP > > >
            ,cds::opt::enable_elimination<true>
        >       Elimination_HP;

        typedef cds::intrusive::TreiberStack< cds::gc::HP, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::HP > > >
            ,cds::opt::enable_elimination<true>
            ,cds::opt::elimination_backoff< cds::backoff::delay_of<2> >
        >       Elimination_HP_2ms;

        typedef cds::intrusive::TreiberStack< cds::gc::HP, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::HP > > >
            ,cds::opt::enable_elimination<true>
            ,cds::opt::elimination_backoff< cds::backoff::delay_of<2> >
            ,cds::opt::stat<cds::intrusive::treiber_stack::stat<> >
        >       Elimination_HP_2ms_stat;

        typedef cds::intrusive::TreiberStack< cds::gc::HP, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::HP > > >
            ,cds::opt::enable_elimination<true>
            ,cds::opt::elimination_backoff< cds::backoff::delay_of<5> >
        >       Elimination_HP_5ms;

        typedef cds::intrusive::TreiberStack< cds::gc::HP, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::HP > > >
            ,cds::opt::enable_elimination<true>
            ,cds::opt::elimination_backoff< cds::backoff::delay_of<5> >
            ,cds::opt::stat<cds::intrusive::treiber_stack::stat<> >
        >       Elimination_HP_5ms_stat;

        typedef cds::intrusive::TreiberStack< cds::gc::HP, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::HP > > >
            ,cds::opt::enable_elimination<true>
            ,cds::opt::elimination_backoff< cds::backoff::delay_of<10> >
        >       Elimination_HP_10ms;

        typedef cds::intrusive::TreiberStack< cds::gc::HP, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::HP > > >
            ,cds::opt::enable_elimination<true>
            ,cds::opt::elimination_backoff< cds::backoff::delay_of<10> >
            ,cds::opt::stat<cds::intrusive::treiber_stack::stat<> >
        >       Elimination_HP_10ms_stat;

        typedef cds::intrusive::TreiberStack< cds::gc::HP, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::HP > > >
            ,cds::opt::enable_elimination<true>
            ,cds::opt::buffer< cds::opt::v::dynamic_buffer<int> >
        >       Elimination_HP_dyn;

        typedef cds::intrusive::TreiberStack< cds::gc::HP, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::HP > > >
            ,cds::opt::enable_elimination<true>
            ,cds::opt::memory_model<cds::opt::v::sequential_consistent>
        >       Elimination_HP_seqcst;

        typedef cds::intrusive::TreiberStack< cds::gc::HP, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::HP > > >
            ,cds::opt::enable_elimination<true>
            ,cds::opt::stat<cds::intrusive::treiber_stack::stat<> >
        >       Elimination_HP_stat;

        typedef cds::intrusive::TreiberStack< cds::gc::HP, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::HP > > >
            ,cds::opt::enable_elimination<true>
            ,cds::opt::stat<cds::intrusive::treiber_stack::stat<> >
            ,cds::opt::buffer< cds::opt::v::dynamic_buffer<int> >
        >       Elimination_HP_dyn_stat;

        typedef cds::intrusive::TreiberStack< cds::gc::HRC, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::HRC > > >
            ,cds::opt::enable_elimination<true>
        >       Elimination_HRC;

        typedef cds::intrusive::TreiberStack< cds::gc::HRC, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::HRC > > >
            ,cds::opt::enable_elimination<true>
            ,cds::opt::buffer< cds::opt::v::dynamic_buffer<int> >
        >       Elimination_HRC_dyn;

        typedef cds::intrusive::TreiberStack< cds::gc::HRC, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::HRC > > >
            ,cds::opt::enable_elimination<true>
            ,cds::opt::stat<cds::intrusive::treiber_stack::stat<> >
        >       Elimination_HRC_stat;

        typedef cds::intrusive::TreiberStack< cds::gc::HRC, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::HRC > > >
            ,cds::opt::enable_elimination<true>
            ,cds::opt::stat<cds::intrusive::treiber_stack::stat<> >
            ,cds::opt::buffer< cds::opt::v::dynamic_buffer<int> >
        >       Elimination_HRC_dyn_stat;

        typedef cds::intrusive::TreiberStack< cds::gc::PTB, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::PTB > > >
            ,cds::opt::enable_elimination<true>
            ,cds::opt::elimination_backoff< cds::backoff::delay_of<2> >
        > Elimination_PTB_2ms;

        typedef cds::intrusive::TreiberStack< cds::gc::PTB, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::PTB > > >
            ,cds::opt::enable_elimination<true>
            ,cds::opt::elimination_backoff< cds::backoff::delay_of<2> >
            ,cds::opt::stat<cds::intrusive::treiber_stack::stat<> >
        > Elimination_PTB_2ms_stat;

        typedef cds::intrusive::TreiberStack< cds::gc::PTB, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::PTB > > >
            ,cds::opt::enable_elimination<true>
            ,cds::opt::elimination_backoff< cds::backoff::delay_of<5> >
        > Elimination_PTB_5ms;

        typedef cds::intrusive::TreiberStack< cds::gc::PTB, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::PTB > > >
            ,cds::opt::enable_elimination<true>
            ,cds::opt::elimination_backoff< cds::backoff::delay_of<5> >
            ,cds::opt::stat<cds::intrusive::treiber_stack::stat<> >
        > Elimination_PTB_5ms_stat;

        typedef cds::intrusive::TreiberStack< cds::gc::PTB, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::PTB > > >
            ,cds::opt::enable_elimination<true>
            ,cds::opt::elimination_backoff< cds::backoff::delay_of<10> >
        > Elimination_PTB_10ms;

        typedef cds::intrusive::TreiberStack< cds::gc::PTB, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::PTB > > >
            ,cds::opt::enable_elimination<true>
            ,cds::opt::elimination_backoff< cds::backoff::delay_of<10> >
            ,cds::opt::stat<cds::intrusive::treiber_stack::stat<> >
        > Elimination_PTB_10ms_stat;

        typedef cds::intrusive::TreiberStack< cds::gc::PTB, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::PTB > > >
            ,cds::opt::enable_elimination<true>
        > Elimination_PTB;

        typedef cds::intrusive::TreiberStack< cds::gc::PTB, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::PTB > > >
            ,cds::opt::enable_elimination<true>
            ,cds::opt::buffer< cds::opt::v::dynamic_buffer<int> >
        > Elimination_PTB_dyn;

        typedef cds::intrusive::TreiberStack< cds::gc::PTB, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::PTB > > >
            ,cds::opt::enable_elimination<true>
            ,cds::opt::stat<cds::intrusive::treiber_stack::stat<> >
        >       Elimination_PTB_stat;

        typedef cds::intrusive::TreiberStack< cds::gc::PTB, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::PTB > > >
            ,cds::opt::enable_elimination<true>
            ,cds::opt::stat<cds::intrusive::treiber_stack::stat<> >
            ,cds::opt::buffer< cds::opt::v::dynamic_buffer<int> >
        >       Elimination_PTB_dyn_stat;

        typedef cds::intrusive::TreiberStack< cds::gc::HP, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::HP > > >
            ,cds::opt::enable_elimination<true>
            ,cds::opt::back_off<cds::backoff::yield>
            ,cds::opt::memory_model<cds::opt::v::relaxed_ordering>
        > Elimination_HP_yield;

        typedef cds::intrusive::TreiberStack< cds::gc::HP, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::HP > > >
            ,cds::opt::enable_elimination<true>
            ,cds::opt::back_off<cds::backoff::pause>
        > Elimination_HP_pause;

        typedef cds::intrusive::TreiberStack< cds::gc::HP, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::HP > > >
            ,cds::opt::enable_elimination<true>
            ,cds::opt::back_off<
                cds::backoff::exponential<
                    cds::backoff::pause,
                    cds::backoff::yield
                >
            >
        > Elimination_HP_exp;

        typedef cds::intrusive::TreiberStack< cds::gc::HRC, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::HRC > > >
            ,cds::opt::enable_elimination<true>
            ,cds::opt::back_off<cds::backoff::yield>
        >  Elimination_HRC_yield;

        typedef cds::intrusive::TreiberStack< cds::gc::HRC, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::HRC > > >
            ,cds::opt::enable_elimination<true>
            ,cds::opt::back_off<cds::backoff::pause>
            ,cds::opt::memory_model<cds::opt::v::relaxed_ordering>
        > Elimination_HRC_pause;

        typedef cds::intrusive::TreiberStack< cds::gc::HRC, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::HRC > > >
            ,cds::opt::enable_elimination<true>
            ,cds::opt::back_off<
                cds::backoff::exponential<
                    cds::backoff::pause,
                    cds::backoff::yield
                >
            >
        > Elimination_HRC_exp;

        typedef cds::intrusive::TreiberStack< cds::gc::PTB, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::PTB > > >
            ,cds::opt::enable_elimination<true>
            ,cds::opt::back_off<cds::backoff::yield>
        >  Elimination_PTB_yield;

        typedef cds::intrusive::TreiberStack< cds::gc::PTB, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::PTB > > >
            ,cds::opt::enable_elimination<true>
            ,cds::opt::back_off<cds::backoff::pause>
        > Elimination_PTB_pause;

        typedef cds::intrusive::TreiberStack< cds::gc::PTB, T
            ,cds::intrusive::opt::hook< cds::intrusive::single_link::base_hook< cds::opt::gc< cds::gc::PTB > > >
            ,cds::opt::enable_elimination<true>
            ,cds::opt::back_off<
                cds::backoff::exponential<
                    cds::backoff::pause,
                    cds::backoff::yield
                >
            >
            ,cds::opt::memory_model<cds::opt::v::relaxed_ordering>
        > Elimination_PTB_exp;

    // FCStack
        typedef cds::intrusive::FCStack< T > FCStack_slist;

        struct traits_FCStack_stat:
            public cds::intrusive::fcstack::make_traits<
                cds::opt::stat< cds::intrusive::fcstack::stat<> >
            >::type
        {};
        struct traits_FCStack_elimination:
            public cds::intrusive::fcstack::make_traits<
            cds::opt::enable_elimination< true >
            >::type
        {};
        struct traits_FCStack_elimination_stat:
            public cds::intrusive::fcstack::make_traits<
                cds::opt::stat< cds::intrusive::fcstack::stat<> >,
                cds::opt::enable_elimination< true >
            >::type
        {};

        struct traits_FCStack_mutex_stat:
            public cds::intrusive::fcstack::make_traits<
                cds::opt::stat< cds::intrusive::fcstack::stat<> >
                ,cds::opt::lock_type< std::mutex >
            >::type
        {};
        struct traits_FCStack_mutex_elimination:
            public cds::intrusive::fcstack::make_traits<
                cds::opt::enable_elimination< true >
                ,cds::opt::lock_type< std::mutex >
            >::type
        {};
        struct traits_FCStack_mutex_elimination_stat:
            public cds::intrusive::fcstack::make_traits<
                cds::opt::stat< cds::intrusive::fcstack::stat<> >
                ,cds::opt::enable_elimination< true >
                ,cds::opt::lock_type< std::mutex >
            >::type
        {};

        typedef cds::intrusive::FCStack< T, boost::intrusive::slist< T >, traits_FCStack_stat > FCStack_slist_stat;
        typedef cds::intrusive::FCStack< T, boost::intrusive::slist< T >, traits_FCStack_elimination > FCStack_slist_elimination;
        typedef cds::intrusive::FCStack< T, boost::intrusive::slist< T >, traits_FCStack_elimination_stat > FCStack_slist_elimination_stat;
        typedef cds::intrusive::FCStack< T, boost::intrusive::slist< T >, traits_FCStack_mutex_stat > FCStack_slist_mutex_stat;
        typedef cds::intrusive::FCStack< T, boost::intrusive::slist< T >, traits_FCStack_mutex_elimination > FCStack_slist_mutex_elimination;
        typedef cds::intrusive::FCStack< T, boost::intrusive::slist< T >, traits_FCStack_mutex_elimination_stat > FCStack_slist_mutex_elimination_stat;
        typedef cds::intrusive::FCStack< T, boost::intrusive::list< T > > FCStack_list;
        typedef cds::intrusive::FCStack< T, boost::intrusive::list< T >, traits_FCStack_stat > FCStack_list_stat;
        typedef cds::intrusive::FCStack< T, boost::intrusive::list< T >, traits_FCStack_elimination > FCStack_list_elimination;
        typedef cds::intrusive::FCStack< T, boost::intrusive::list< T >, traits_FCStack_elimination_stat > FCStack_list_elimination_stat;
        typedef cds::intrusive::FCStack< T, boost::intrusive::list< T >, traits_FCStack_mutex_stat > FCStack_list_mutex_stat;
        typedef cds::intrusive::FCStack< T, boost::intrusive::list< T >, traits_FCStack_mutex_elimination > FCStack_list_mutex_elimination;
        typedef cds::intrusive::FCStack< T, boost::intrusive::list< T >, traits_FCStack_mutex_elimination_stat > FCStack_list_mutex_elimination_stat;

    // MichaelDeque, left side
        typedef details::MichaelDequeL< cds::gc::HP, T
            ,cds::intrusive::opt::hook< cds::intrusive::michael_deque::base_hook< cds::opt::gc< cds::gc::HP > > >
        > MichaelDequeL_HP;
        typedef details::MichaelDequeL< cds::gc::HP, T
            ,cds::intrusive::opt::hook< cds::intrusive::michael_deque::base_hook< cds::opt::gc< cds::gc::HP > > >
            ,cds::opt::memory_model<cds::opt::v::sequential_consistent>
        > MichaelDequeL_HP_seqcst;

        typedef details::MichaelDequeL< cds::gc::HP, T
            ,cds::intrusive::opt::hook< cds::intrusive::michael_deque::base_hook< cds::opt::gc< cds::gc::HP > > >
            ,cds::opt::item_counter< cds::atomicity::item_counter >
        > MichaelDequeL_HP_ic;

        typedef details::MichaelDequeL< cds::gc::HP, T
            ,cds::intrusive::opt::hook< cds::intrusive::michael_deque::base_hook< cds::opt::gc< cds::gc::HP > > >
            ,cds::opt::back_off<
                cds::backoff::exponential<
                    cds::backoff::pause,
                    cds::backoff::yield
                >
            >
        > MichaelDequeL_HP_exp;

        typedef details::MichaelDequeL< cds::gc::HP, T
            ,cds::intrusive::opt::hook< cds::intrusive::michael_deque::base_hook< cds::opt::gc< cds::gc::HP > > >
            ,cds::intrusive::opt::hook< cds::intrusive::michael_deque::base_hook< cds::opt::gc< cds::gc::HP > > >
            ,cds::opt::back_off< cds::backoff::yield >
        > MichaelDequeL_HP_yield;

        typedef details::MichaelDequeL< cds::gc::HP, T
            ,cds::intrusive::opt::hook< cds::intrusive::michael_deque::base_hook< cds::opt::gc< cds::gc::HP > > >
            ,cds::opt::stat<cds::intrusive::deque_stat<> >
        > MichaelDequeL_HP_stat;


        typedef details::MichaelDequeL< cds::gc::PTB, T
            ,cds::intrusive::opt::hook< cds::intrusive::michael_deque::base_hook< cds::opt::gc< cds::gc::PTB > > >
        > MichaelDequeL_PTB;
        typedef details::MichaelDequeL< cds::gc::PTB, T
            ,cds::intrusive::opt::hook< cds::intrusive::michael_deque::base_hook< cds::opt::gc< cds::gc::PTB > > >
            ,cds::opt::memory_model<cds::opt::v::sequential_consistent>
        > MichaelDequeL_PTB_seqcst;

        typedef details::MichaelDequeL< cds::gc::PTB, T
            ,cds::intrusive::opt::hook< cds::intrusive::michael_deque::base_hook< cds::opt::gc< cds::gc::PTB > > >
            ,cds::opt::item_counter< cds::atomicity::item_counter >
        > MichaelDequeL_PTB_ic;

        typedef details::MichaelDequeL< cds::gc::PTB, T
            ,cds::intrusive::opt::hook< cds::intrusive::michael_deque::base_hook< cds::opt::gc< cds::gc::PTB > > >
            ,cds::opt::back_off<
                cds::backoff::exponential<
                    cds::backoff::pause,
                    cds::backoff::yield
                >
            >
        > MichaelDequeL_PTB_exp;

        typedef details::MichaelDequeL< cds::gc::PTB, T
            ,cds::intrusive::opt::hook< cds::intrusive::michael_deque::base_hook< cds::opt::gc< cds::gc::PTB > > >
            ,cds::opt::back_off< cds::backoff::yield >
        > MichaelDequeL_PTB_yield;

        typedef details::MichaelDequeL< cds::gc::PTB, T
            ,cds::intrusive::opt::hook< cds::intrusive::michael_deque::base_hook< cds::opt::gc< cds::gc::PTB > > >
            ,cds::opt::stat<cds::intrusive::michael_deque::stat<> >
        > MichaelDequeL_PTB_stat;


    // MichaelDeque, right side
        typedef details::MichaelDequeR< cds::gc::HP, T
            ,cds::intrusive::opt::hook< cds::intrusive::michael_deque::base_hook< cds::opt::gc< cds::gc::HP > > >
        > MichaelDequeR_HP;
        typedef details::MichaelDequeR< cds::gc::HP, T
            ,cds::intrusive::opt::hook< cds::intrusive::michael_deque::base_hook< cds::opt::gc< cds::gc::HP > > >
            ,cds::opt::memory_model<cds::opt::v::sequential_consistent>
        > MichaelDequeR_HP_seqcst;

        typedef details::MichaelDequeR< cds::gc::HP, T
            ,cds::intrusive::opt::hook< cds::intrusive::michael_deque::base_hook< cds::opt::gc< cds::gc::HP > > >
            ,cds::opt::item_counter< cds::atomicity::item_counter >
        > MichaelDequeR_HP_ic;

        typedef details::MichaelDequeR< cds::gc::HP, T
            ,cds::intrusive::opt::hook< cds::intrusive::michael_deque::base_hook< cds::opt::gc< cds::gc::HP > > >
            ,cds::opt::back_off<
                cds::backoff::exponential<
                    cds::backoff::pause,
                    cds::backoff::yield
                >
            >
        > MichaelDequeR_HP_exp;

        typedef details::MichaelDequeR< cds::gc::HP, T
            ,cds::intrusive::opt::hook< cds::intrusive::michael_deque::base_hook< cds::opt::gc< cds::gc::HP > > >
            ,cds::opt::back_off< cds::backoff::yield >
        > MichaelDequeR_HP_yield;

        typedef details::MichaelDequeR< cds::gc::HP, T
            ,cds::opt::stat< cds::intrusive::michael_deque::stat<> >
        > MichaelDequeR_HP_stat;

        typedef details::MichaelDequeR< cds::gc::PTB, T
            ,cds::intrusive::opt::hook< cds::intrusive::michael_deque::base_hook< cds::opt::gc< cds::gc::PTB > > >
        > MichaelDequeR_PTB;
        typedef details::MichaelDequeR< cds::gc::PTB, T
            ,cds::intrusive::opt::hook< cds::intrusive::michael_deque::base_hook< cds::opt::gc< cds::gc::PTB > > >
            ,cds::opt::memory_model<cds::opt::v::sequential_consistent>
        > MichaelDequeR_PTB_seqcst;

        typedef details::MichaelDequeR< cds::gc::PTB, T
            ,cds::intrusive::opt::hook< cds::intrusive::michael_deque::base_hook< cds::opt::gc< cds::gc::PTB > > >
            ,cds::opt::item_counter< cds::atomicity::item_counter >
        > MichaelDequeR_PTB_ic;

        typedef details::MichaelDequeR< cds::gc::PTB, T
            ,cds::intrusive::opt::hook< cds::intrusive::michael_deque::base_hook< cds::opt::gc< cds::gc::PTB > > >
            ,cds::opt::back_off<
                cds::backoff::exponential<
                    cds::backoff::pause,
                    cds::backoff::yield
                >
            >
        > MichaelDequeR_PTB_exp;

        typedef details::MichaelDequeR< cds::gc::PTB, T
            ,cds::intrusive::opt::hook< cds::intrusive::michael_deque::base_hook< cds::opt::gc< cds::gc::PTB > > >
            ,cds::opt::stat< cds::intrusive::deque_stat<> >
        > MichaelDequeR_PTB_stat;

        typedef details::MichaelDequeR< cds::gc::PTB, T
            ,cds::intrusive::opt::hook< cds::intrusive::michael_deque::base_hook< cds::opt::gc< cds::gc::PTB > > >
            ,cds::opt::back_off< cds::backoff::yield >
        > MichaelDequeR_PTB_yield;


        // std::stack
        typedef details::StdStack< T, std::stack< T* >, std::mutex >  StdStack_Deque_Mutex;
        typedef details::StdStack< T, std::stack< T* >, cds::lock::Spin > StdStack_Deque_Spin;
        typedef details::StdStack< T, std::stack< T*, std::vector<T*> >, std::mutex >  StdStack_Vector_Mutex;
        typedef details::StdStack< T, std::stack< T*, std::vector<T*> >, cds::lock::Spin > StdStack_Vector_Spin;
        typedef details::StdStack< T, std::stack< T*, std::list<T*> >, std::mutex >  StdStack_List_Mutex;
        typedef details::StdStack< T, std::stack< T*, std::list<T*> >, cds::lock::Spin > StdStack_List_Spin;

    };
} // namespace istack

namespace std {
    static inline ostream& operator <<( ostream& o, cds::intrusive::treiber_stack::stat<> const& s )
    {
        return o << "\tStatistics:\n"
            << "\t                    Push: " << s.m_PushCount.get()              << "\n"
            << "\t                     Pop: " << s.m_PopCount.get()               << "\n"
            << "\t         Push contention: " << s.m_PushRace.get()               << "\n"
            << "\t          Pop contention: " << s.m_PopRace.get()                << "\n"
            << "\t   m_ActivePushCollision: " << s.m_ActivePushCollision.get()    << "\n"
            << "\t   m_PassivePopCollision: " << s.m_PassivePopCollision.get()    << "\n"
            << "\t    m_ActivePopCollision: " << s.m_ActivePopCollision.get()     << "\n"
            << "\t  m_PassivePushCollision: " << s.m_PassivePushCollision.get()   << "\n"
            << "\t     m_EliminationFailed: " << s.m_EliminationFailed.get()      << "\n";
    }

    static inline ostream& operator <<( ostream& o, cds::intrusive::treiber_stack::empty_stat const& s )
    {
        return o;
    }

    static inline ostream& operator <<( ostream& o, cds::intrusive::fcstack::empty_stat const& s )
    {
        return o;
    }

    static inline ostream& operator <<( ostream& o, cds::intrusive::fcstack::stat<> const& s )
    {
        return o << "\tStatistics:\n"
            << "\t                    Push: " << s.m_nPush.get()              << "\n"
            << "\t                     Pop: " << s.m_nPop.get()               << "\n"
            << "\t               FailedPop: " << s.m_nFailedPop.get()         << "\n"
            << "\t  Collided push/pop pair: " << s.m_nCollided.get()          << "\n"
            << "\tFlat combining statistics:\n"
            << "\t        Combining factor: " << s.combining_factor()         << "\n"
            << "\t         Operation count: " << s.m_nOperationCount.get()    << "\n"
            << "\t      Combine call count: " << s.m_nCombiningCount.get()    << "\n"
            << "\t        Compact pub-list: " << s.m_nCompactPublicationList.get() << "\n"
            << "\t   Deactivate pub-record: " << s.m_nDeactivatePubRecord.get()    << "\n"
            << "\t     Activate pub-record: " << s.m_nActivatePubRecord.get()    << "\n"
            << "\t       Create pub-record: " << s.m_nPubRecordCreated.get()  << "\n"
            << "\t       Delete pub-record: " << s.m_nPubRecordDeteted.get()  << "\n"
            << "\t      Acquire pub-record: " << s.m_nAcquirePubRecCount.get()<< "\n"
            << "\t      Release pub-record: " << s.m_nReleasePubRecCount.get()<< "\n";
    }

} // namespace std

#endif // #ifndef __CDSUNIT_INTRUSIVE_STACK_TYPES_H
