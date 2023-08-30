#ifndef CDSLIB_GC_DETAILS_SCAN_FUNCTIONS_H
#define CDSLIB_GC_DETAILS_SCAN_FUNCTIONS_H

#include <algorithm>
#include <functional>

#include <cds/gc/details/hp_common.h>
#include <cds/gc/details/hp_client_record.h>

namespace cds { namespace gc { namespace hp { namespace details {

struct HpMechanizmPart
{
    HpMechanizmPart(thread_data* pThreadRec, const atomics::atomic<client_record*>& thread_list)
        : pThreadRec{pThreadRec}
        , thread_list{thread_list}
    {}
    
    // hazards and retired from client who call scan
    thread_data* pThreadRec;
    // All hazards
    const atomics::atomic<client_record*>& thread_list;
};


// cppcheck-suppress functionConst
template<class TVectorLikeEmptyContainer, typename TContainerCreator>
void classic_scan(HpMechanizmPart hazard_data, TContainerCreator&& container_creator)
{
    client_record *pRec = static_cast<client_record *>(hazard_data.pThreadRec);

    CDS_HPSTAT(++pThreadRec->scan_count_);

    // Stage 1: Scan HP list and insert non-null values in plist

    client_record *pNode = hazard_data.thread_list.load(atomics::memory_order_acquire);
    TVectorLikeEmptyContainer plist = container_creator();

    while (pNode)
    {
        if (pNode->owner_rec_.load(std::memory_order_relaxed) != nullptr)
        {
            // for (size_t i = 0; i < pNode->hazards_.capacity() get_hazard_ptr_count(); ++i)
            for (size_t i = 0; i < pNode->hazards_.capacity(); ++i)
            {
                void *hptr = pNode->hazards_[i].get();
                if (hptr) {
                    plist.push_back(hptr);
                }
            }
        }
        pNode = pNode->next_;
    }

    // Sort plist to simplify search in
    std::sort(plist.begin(), plist.end());

    // Stage 2: Search plist
    retired_array &retired = pRec->retired_;

    retired_ptr *first_retired = retired.first();
    retired_ptr *last_retired = retired.last();

    {
        auto itBegin = plist.begin();
        auto itEnd = plist.end();
        retired_ptr *insert_pos = first_retired;
        for (retired_ptr *it = first_retired; it != last_retired; ++it)
        {
            if (std::binary_search(itBegin, itEnd, first_retired->m_p))
            {
                if (insert_pos != it)
                    *insert_pos = *it;
                ++insert_pos;
            }
            else
            {
                it->free();
                CDS_HPSTAT(++pRec->free_count_);
            }
        }

        retired.reset(insert_pos - first_retired);
    }
}

template<class TVectorLikeEmptyContainer, typename TContainerCreator>
void inplace_scan(HpMechanizmPart hazard_data, TContainerCreator&& container_creator)
{
    client_record *pRec = static_cast<client_record *>(hazard_data.pThreadRec);

    // CDS_HAZARDPTR_STATISTIC( ++m_Stat.m_ScanCallCount )

    // In-place scan algo uses LSB of retired ptr as a mark for internal purposes.
    // It is correct if all retired pointers are ar least 2-byte aligned (LSB is zero).
    // If it is wrong, we use classic scan algorithm

    // Check if all retired pointers has zero LSB
    // LSB is used for marking pointers that cannot be deleted yet
    retired_ptr *first_retired = pRec->retired_.first();
    retired_ptr *last_retired = pRec->retired_.last();
    if (first_retired == last_retired)
        return;

    for (auto it = first_retired; it != last_retired; ++it)
    {
        if (it->m_n & 1)
        {
            // found a pointer with LSB bit set - use classic_scan
            classic_scan<TVectorLikeEmptyContainer>(hazard_data, std::move(container_creator));
            return;
        }
    }

    CDS_HPSTAT(++pThreadRec->scan_count_);

    // Sort retired pointer array
    std::sort(first_retired, last_retired, retired_ptr::less);

    // Check double free
#ifdef _DEBUG
    {
        auto it = first_retired;
        auto itPrev = it;
        while (++it != last_retired)
        {
            assert(itPrev->m_p < it->m_p);
            itPrev = it;
        }
    }
#endif

    // Search guarded pointers in retired array
    client_record *pNode = hazard_data.thread_list.load(atomics::memory_order_acquire);

    {
        retired_ptr dummy_retired;
        while (pNode)
        {
            if (pNode->owner_rec_.load(atomics::memory_order_relaxed) != nullptr)
            {
                thread_hp_storage &hpstg = pNode->hazards_;

                for (auto hp = hpstg.begin(), end = hpstg.end(); hp != end; ++hp)
                {
                    void *hptr = hp->get(atomics::memory_order_relaxed);
                    if (hptr)
                    {
                        dummy_retired.m_p = hptr;
                        retired_ptr *it = std::lower_bound(first_retired, last_retired, dummy_retired, retired_ptr::less);
                        if (it != last_retired && it->m_p == hptr)
                        {
                            // Mark retired pointer as guarded
                            it->m_n |= 1;
                        }
                    }
                }
            }
            pNode = pNode->next_;
        }
    }

    // Move all marked pointers to head of array
    {
        retired_ptr *insert_pos = first_retired;
        for (retired_ptr *it = first_retired; it != last_retired; ++it)
        {
            if (it->m_n & 1)
            {
                it->m_n &= ~uintptr_t(1);
                if (insert_pos != it)
                    *insert_pos = *it;
                ++insert_pos;
            }
            else
            {
                // Retired pointer may be freed
                it->free();
                CDS_HPSTAT(++pRec->free_count_);
            }
        }
        const size_t nDeferred = insert_pos - first_retired;
        pRec->retired_.reset(nDeferred);
    }
}
            }
        }
    }} // namespace cds::gc::hp::details

#endif // CDSLIB_GC_DETAILS_SCAN_FUNCTIONS_H