// The allocator the rest of the library will route every allocation through.
//
// M0. The engine installs its own through GrannySetAllocator during startup, so
// this is the first entry point of the 54 that any run reaches, and it has to
// hold the two callbacks before anything else can allocate.

#include <gr2/granny.h>

#include "Trace.h"

extern "C"
{

GR2_API( void ) GrannySetAllocator( granny_allocate_callback *AllocateCallback,
                                    granny_deallocate_callback *DeallocateCallback )
{
	GR2_TRACE( "AllocateCallback={} DeallocateCallback={}", AllocateCallback, DeallocateCallback );
}

GR2_API( void ) GrannyGetAllocator( granny_allocate_callback **AllocateCallback,
                                    granny_deallocate_callback **DeallocateCallback )
{
	GR2_TRACE( "AllocateCallback={} DeallocateCallback={}", AllocateCallback, DeallocateCallback );
}

}
