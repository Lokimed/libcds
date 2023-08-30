#include <cds/gc/details/hp_default_memory_allocator.h>

namespace cds { namespace gc { namespace hp { namespace details {

        void * default_alloc_memory( size_t size )
        {
            return new uintptr_t[( size + sizeof( uintptr_t ) - 1 ) / sizeof( uintptr_t) ];
        }

        void default_free_memory( void* p )
        {
            delete[] reinterpret_cast<uintptr_t*>( p );
        }

}}}} // namespace cds::gc::hp::details