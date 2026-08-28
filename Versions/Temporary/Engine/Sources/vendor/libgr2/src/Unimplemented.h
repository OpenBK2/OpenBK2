#pragma once

// What an entry point does until it is written.
//
// Every one of the 54 stubs routes through here before returning its neutral
// value, so that a run against this library says which entry points it actually
// reached and in what order. That is the same question the Linux stub in
// vendor/granny answers, and the answer is what orders the milestones: the
// engine references all 54, but a reference is not a call, and some of them sit
// on paths the shipped data never takes.
//
// GR2_STUB is a statement, not an expression, so a stub with a return value
// still writes its own return and stays readable as the real body that will
// replace it.

namespace NGr2
{

//! Record that pszFunction was called and has no implementation behind it.
//!
//! Reports each entry point once, on stderr, rather than every call: several of
//! these run per bone per frame, and a flood buries the loading sequence, which
//! is the part worth reading. Safe to call from any of the stubs at any time,
//! including before main.
void ReportUnimplemented( const char *pszFunction );

}

#define GR2_STUB() ::NGr2::ReportUnimplemented( __func__ )
