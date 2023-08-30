#ifndef CDSLIB_GC_DETAILS_HP_TLS_MANAGERS_H
#define CDSLIB_GC_DETAILS_HP_TLS_MANAGERS_H

#include <cds/gc/details/hp_common.h>

namespace cds { namespace gc { namespace hp { namespace details {
    using namespace cds::gc::hp::common;

    /// Default TLS manager
    /**
        By default, HP stores its data in TLS.
        This class provides such behavoiur.
    */
    class DefaultTLSManager {
#ifndef CDS_DISABLE_CLASS_TLS_INLINE
        // GCC, CLang
    public:
        /// Get HP data for current thread
        static thread_data* getTLS()
        {
            return tls_;
        }
        /// Set HP data for current thread
        static void setTLS(thread_data* td)
        {
            tls_ = td;
        }
    private:
        //@cond
        static thread_local thread_data* tls_;
        //@endcond
#else   
        // MSVC
    public:
        static CDS_EXPORT_API thread_data* getTLS() noexcept;
        static CDS_EXPORT_API void setTLS(thread_data*) noexcept;
#endif
    };

    //@cond
    // Strange thread manager for testing purpose only!
    class StrangeTLSManager {
    public:
        static CDS_EXPORT_API thread_data* getTLS();
        static CDS_EXPORT_API void setTLS(thread_data*);
    };
    //@endcond

}}}} // namespace cds::gc::hp::details

#endif // #ifndef CDSLIB_GC_DETAILS_HP_COMMON_H