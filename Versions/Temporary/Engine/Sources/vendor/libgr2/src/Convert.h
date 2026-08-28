#pragma once

// Turning what a file contains into what the engine reads.
//
// The one entry point, because everything below it is reached from the root
// object and there is nothing else worth exposing. See Convert.cpp for what the
// conversion consists of and why it is not simply a pointer into the file.

#include "File.h"

namespace NGr2
{

//! Convert a loaded file's root object into a granny_file_info.
//!
//! Built once and kept on the file, so repeated calls are free and every caller
//! gets the same pointers, which the engine relies on. Null if the file's type
//! tree does not read, and the failure is remembered rather than retried.
void *ConvertFileInfo( granny_file &file );

}
