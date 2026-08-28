#pragma once

// The type tree: how a GR2 describes its own structures.
//
// Every object in a file is preceded by nothing that says what it is. What says
// it is a chain of type definitions, each naming a member, its kind, and the
// type it refers to, terminated by an End marker. Members are resolved through
// that chain by name, never by a hardcoded offset, which is the single most
// important decision in reading these files: four struct tags appear in this
// game's data and more appear in other games, and a reader written against fixed
// offsets produces plausible garbage on all but the one it was written for.
//
// Sizes here are the sizes on disk, with 32-bit pointers, whatever the host is.
// The sizes the engine sees are a different question, answered in Structures.h,
// and GrannyGetMemberTypeSize below answers that second one because that is what
// its callers mean by it.

#include "File.h"

#include <string>
#include <vector>

namespace NGr2
{

//! The size of one type definition entry on disk: Type, Name, ReferenceType,
//! ArrayWidth, Extra[3], Ignored.
constexpr uint32_t TYPE_DEFINITION_DISK_SIZE = 32;

//! Every member of the type definition at typeRef, in order, with disk offsets.
//!
//! Null when the definition is malformed, which includes running off the end of
//! its section and having no End marker. Cached on the file, so the returned
//! vector lives as long as it does and repeated lookups are free.
const std::vector<SMember> *ReadType( granny_file &file, const SReference &typeRef );

//! What an object of this type occupies on disk. 0 if the type is malformed.
uint32_t DiskObjectSize( granny_file &file, const SReference &typeRef );

//! The member called pszName, or null. Linear, because these lists are short and
//! the alternative is a map per type that is built once and read twice.
const SMember *FindMember( const std::vector<SMember> &members, const char *pszName );

}
