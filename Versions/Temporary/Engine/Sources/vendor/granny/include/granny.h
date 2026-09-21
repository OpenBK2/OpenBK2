#pragma once

// The one place the engine reaches Granny through, and now the one place that
// says which header answers.
//
// It used to be granny211.h, RAD Game Tools' own 6,000-line SDK header out of
// the third_party/uesp-esoapps submodule. libgr2 is what actually implements
// every one of those entry points (see cmake/granny.cmake), and its public
// header declares the same ABI: the same entry points with the same signatures
// and decoration, the same record layouts with the same member names, and the
// same version macros. So the engine compiles against the library that answers
// it rather than against the header of the library it replaced.
//
// The engine's twenty-odd call sites are unchanged, and this file stays a
// redirect rather than being deleted at every one of them, so that there is
// still a single line to edit when the Granny-shaped names are finally retired
// for a format-neutral interface.
#include <gr2/granny.h>
