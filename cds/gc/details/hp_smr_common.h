#ifndef CDSLIB_GC_DETAILS_SMR_COMMON_H
#define CDSLIB_GC_DETAILS_SMR_COMMON_H

#include <cstddef>

#include <cds/algo/atomic.h>

#include <cds/gc/details/hp_client_record.h>
#include <cds/gc/details/hp_default_memory_allocator.h>

namespace cds { namespace gc { namespace hp { namespace details {

struct smr_common_data {

    /// \p smr::scan() strategy
    enum scan_type {
        classic,    ///< classic scan as described in Michael's works (see smr::classic_scan())
        inplace     ///< inplace scan without allocation (see smr::inplace_scan())
    };

    CDS_EXPORT_API smr_common_data(size_t nHazardPtrCount, size_t nMaxThreadCount, size_t nMaxRetiredPtrCount, scan_type nScanType );   

protected:
    atomics::atomic<client_record*> thread_list_;   ///< Head of thread list

    size_t const hazard_ptr_count_;      ///< max count of thread's hazard pointer
    size_t const max_thread_count_;      ///< max count of thread
    size_t const max_retired_ptr_count_; ///< max count of retired ptr per thread

    scan_type const scan_type_;             ///< scan type (see \ref scan_type enum)
};

CDS_EXPORT_API client_record* create_client_data(size_t hazard_ptr_count, size_t max_retired_ptr_count, s_alloc_memory_t s_alloc_memory);

CDS_EXPORT_API client_record* alloc_thread_data(
    atomics::atomic<client_record*>& thread_list,
    size_t hazard_ptr_count, size_t max_retired_ptr_count, s_alloc_memory_t s_alloc_memory);

}}}} // namespace cds::gc::hp::details

#endif // CDSLIB_GC_DETAILS_SMR_COMMON_H