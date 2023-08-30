#ifndef CDSLIB_GC_DETAILS_HP_DEFAULT_MEMORY_ALLOCATORS_H
#define CDSLIB_GC_DETAILS_HP_DEFAULT_MEMORY_ALLOCATORS_H

#include <cstdint>
#include <cstddef>

namespace cds { namespace gc { namespace hp { namespace details {

        void * default_alloc_memory( size_t size );
        void default_free_memory( void* p );

        using s_alloc_memory_t = void* (*)( size_t size );
        using s_free_memory_t = void (*)( void* p );

}}}} // namespace cds::gc::hp::details

#endif // CDSLIB_GC_DETAILS_HP_DEFAULT_MEMORY_ALLOCATORS_H