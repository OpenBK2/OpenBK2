#pragma once

#include "System_export.h"


namespace NSdl
{
	//! Bring SDL's video subsystem up, once, and report whether it is usable.
	//!
	//! Call this at the top of anything that touches SDL video rather than
	//! initialising somewhere central. Initialisation order here is not a
	//! straight line: subsystems register through a call graph rather than a
	//! list, a static constructor or a mod can reach video before anything in
	//! main does, and third_party/flessd already brings up SDL's audio from
	//! wherever the sound engine is first touched. Nothing can assume it runs
	//! first, so every entry point asks and the answer is computed once.
	SYSTEM_EXPORT bool EnsureVideo();
}
