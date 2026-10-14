#pragma once

// The identity of the simulation code that is running, for stamping into files
// whose meaning depends on it.
//
// A replay is the case this exists for. It records player commands and state
// checksums, and a playback only means anything against the build that recorded
// it: change the simulation and the recording diverges on every run,
// identically, which is indistinguishable from a reproducible defect by
// inspection. Two debugging sessions were spent on exactly that before anything
// stamped a replay.
//
// This is deliberately the linker's build identity rather than a version string.
// REVISION_NUMBER_STR and BUILD_DATE_TIME_STR exist already and are useless for
// the purpose: a local build reports "0.0.0.0" and the Unix epoch, so every
// developer's every build claims to be the same one, which is the only case that
// matters here. A build id changes whenever the image changes and needs nothing
// from the build system to be correct.

#include "System_export.h"

#include <string>

namespace NBuildStamp
{
// A digest of the build ids of the modules the simulation is made of, as
// lowercase hex, or an empty string when none of them carries a build
// identity. Empty is a legitimate answer that callers have to treat as
// "unknown" rather than as a value to compare: a stripped ELF without
// --build-id and a PE built without debug information both reach it.
//
// Per module and not per executable, because every subsystem here is a
// shared library and `Game` is a thin executable over them. Rebuilding
// `AILogic` leaves the executable's own build id untouched, so stamping that
// would have been silent for most simulation changes, which is precisely the
// case this is for. Measured, not assumed: a build that changed `GameX` and
// `System` produced a byte-identical `Game`.
//
// A fixed list of modules rather than everything loaded. Hashing every
// loaded object would fold in the graphics driver and the Vulkan ICDs, which
// are dlopened and closed by the loader and differ with the machine's GPU,
// so the digest would differ between two machines running the same build and
// would cry wolf on exactly the cross-machine comparison the replay harness
// exists for. The cost is that a simulation-relevant module added later has
// to be added to the list; see the note in the .cpp.
//
// On ELF each module contributes its GNU build id, the hash the linker puts
// in a PT_NOTE segment; on PE it contributes the PDB GUID and age from the
// debug directory. Both identify one link of one image.
//
// Computed once on first call and cached, since the answer cannot change
// while the process runs.
SYSTEM_EXPORT const std::string &GetSimulationImageId();

// How this image was built, as a short label: "release", "debug",
// "release+tsan", "debug+asan+ubsan" and so on.
//
// Recorded beside the image id because the id alone answers the wrong
// question. Every build identifier available is weak in some direction. A
// date changes when nothing did. A git revision misses uncommitted work,
// compiler and flag changes and system headers, and moves for a change to a
// document. The build id catches all of those but also differs between a
// release build and a sanitizer build of identical sources, and comparing
// those two is the main thing the replay harness is for -- so an id
// mismatch on its own would fire on nearly every verification run and teach
// the reader to skip the line.
//
// The configuration cannot separate a source change from a flag change
// either. What it does is let the report state the one thing that is always
// true: here is what recorded it, here is what is running, and these are
// the ways that can happen. The reader knows which of those they intended.
SYSTEM_EXPORT const std::string &GetBuildConfiguration();
}
