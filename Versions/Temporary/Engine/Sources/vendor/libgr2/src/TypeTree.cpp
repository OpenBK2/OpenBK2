// The type tree, and the two size queries the engine asks of it.
//
// M1. Every .gr2 carries a description of its own structures, and members are
// resolved through that rather than through hardcoded offsets: this game's files
// are one dialect, but the dialect is what the tree says it is, not what a
// struct definition compiled today happens to be.
//
// The engine calls these two when walking vertex data, where the size of a
// member is what advances the cursor.

#include <gr2/granny.h>

#include "Trace.h"

extern "C"
{

GR2_API( granny_int32x ) GrannyGetMemberTypeSize( granny_data_type_definition const *MemberType )
{
	GR2_TRACE( "MemberType={}", MemberType );
	return 0;
}

GR2_API( granny_int32x )
	GrannyGetTotalObjectSize( granny_data_type_definition const *TypeDefinition )
{
	GR2_TRACE( "TypeDefinition={}", TypeDefinition );
	return 0;
}

}
