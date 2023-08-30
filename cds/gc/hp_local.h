// This is try to keep all guard non per thread by per class
// So every container can have own guard
// So if we will have more than one container there will be not harazd guards intersection


namespace cds { namespace gc {
    /// Hazard pointer implementation details
    namespace hp {
        namespace details {

        //@cond
        /// Hazard Pointer SMR (Safe Memory Reclamation)
        class local_basic_smr {
            
        };

}}}} // namespace cds::gc::hp::details