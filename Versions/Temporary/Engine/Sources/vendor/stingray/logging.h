#pragma once

// Make sure the trace this library writes actually goes somewhere.
//
// This is a static library, and a linker takes an object file out of one only to
// resolve a symbol something asked for. logging.cpp defines InitLogging() and an
// object whose constructor calls it, and for as long as nothing referred to
// either, logging.cpp.obj sat in stingray.lib and was never linked into
// anything: the constructor never ran, spdlog's default logger stayed the one
// that writes to stdout at info level, and all 269 trace lines in this library
// were dropped. This is the same hazard the editor modules are shared libraries
// to avoid, noted in AGENTS.md, and it is not a thing a static initialiser can
// solve for itself.
//
// So every source file in this library includes this header, and the object
// below is what refers to InitLogging. One reference from any translation unit
// that does get linked is enough to pull logging.cpp.obj in with it, and
// InitLogging is idempotent, so more than one costs nothing.

void InitLogging();

namespace
{

struct SStingrayLoggingInstaller
{
	SStingrayLoggingInstaller() { InitLogging(); }
};

const SStingrayLoggingInstaller g_StingrayLoggingInstaller;

}
