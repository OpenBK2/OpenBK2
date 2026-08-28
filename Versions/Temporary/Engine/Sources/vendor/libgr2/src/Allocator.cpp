// Where the engine would install its own allocator, if it ever did.
//
// It does not. InitializeGrannyMemoryMap in 3Dmotor/GrannyMemoryMap.cpp is the
// only caller of either of these, and nothing calls InitializeGrannyMemoryMap,
// so both are linked and never reached. A traced run confirms it: the first call
// the game makes is GrannyReadEntireFileFromMemory, with nothing before it.
//
// So these two stay stubs, and this library allocates for itself. They still
// have to exist and be exported, because the engine links them, and they still
// have to work if that dead code is ever revived, which is why the milestone
// they belong to is not "never" but "whenever something needs them".

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
