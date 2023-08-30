#ifndef CDSLIB_GC_DETAILS_CLIENT_RECORD_H
#define CDSLIB_GC_DETAILS_CLIENT_RECORD_H                

#include <cds/gc/details/hp_common.h>

namespace cds { namespace gc { namespace hp { namespace details {

    // In classic hp realization client record is one per thread and better to have alias `using thread_record = client_record` for clarify
    // But also possible when this is record for one client.
    // One thread can have more that one client (really this is strange situation, but possible) 
    struct client_record: thread_data
    {
        // next hazard ptr record in list
        client_record* next_ = nullptr;
        // FIXME?? Why need two variables for detect is record free or busy?
        // Owner thread_record; nullptr - the record is free (not owned)
        atomics::atomic<client_record*>  owner_rec_;
        // true if record is free (not owned)
        atomics::atomic<bool> free_{ false };

        client_record( guard* guards, size_t guard_count, retired_ptr* retired_arr, size_t retired_capacity )
            : thread_data( guards, guard_count, retired_arr, retired_capacity ), owner_rec_(this)
            {}
    };

}}}} // namespace cds::gc::hp::details

#endif // CDSLIB_GC_DETAILS_CLIENT_RECORD_H