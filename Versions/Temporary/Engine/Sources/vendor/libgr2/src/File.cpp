// The container: reading a .gr2 and handing back the object tree inside it.
//
// M1. Sections, fixups, marshalling, and the two Oodle codecs this game's files
// use, then a parse into structures this library owns. The files store 32-bit
// pointers, so nothing here memory-maps: it allocates and populates, which is
// what lets x86 and x64 share one path.
//
// GrannyReadEntireFileFromMemory is the entry point the engine reaches first
// after the allocator, since every model arrives out of a .pak already in
// memory. GrannyReadEntireFile exists for the same reason it does in Granny, a
// loose file on disk, and is the cheaper of the two to write.

#include <gr2/granny.h>

#include "Trace.h"

extern "C"
{

GR2_API( granny_file * ) GrannyReadEntireFile( char const *FileName )
{
	GR2_TRACE( "FileName={}", FileName );
	return 0;
}

GR2_API( granny_file * ) GrannyReadEntireFileFromMemory( granny_int32x MemorySize,
                                                         void const *Memory )
{
	GR2_TRACE( "MemorySize={} Memory={}", MemorySize, Memory );
	return 0;
}

GR2_API( void ) GrannyFreeFile( granny_file *File )
{
	GR2_TRACE( "File={}", File );
}

GR2_API( granny_file_info * ) GrannyGetFileInfo( granny_file *File )
{
	GR2_TRACE( "File={}", File );
	return 0;
}

}
