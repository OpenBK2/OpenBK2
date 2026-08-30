#pragma once

// Which file an object came from, so that a log can be read after the fact.
//
// A warning that says a skeleton of 21 bones is at 0xbdf270 answers nothing. The
// address is different next run, every human infantry skeleton in this game has
// exactly 21 bones, and every one of them is named "Hip" after its root joint.
// What identifies a resource is the bytes it was loaded from, so every file that
// arrives here is hashed and its converted objects are registered against that
// hash. A warning can then name the file, and scripts/port/gr2whois.py turns the
// hash back into a pak entry, the .xdb record that carries its GUID, and the
// units that reference it.
//
// The engine never says what it is loading. CGrannyMemFileLoader::RecalcValue
// pulls a resource out of a pak through the VFS and hands over a naked buffer,
// so the content hash and the exporter's own FromFileName are the only identity
// there is.
//
// Shared with the forwarding shim in Shim.cpp, which registers the real DLL's
// structures through the same three calls. That is the point of putting this
// here rather than inside the entry points: both implementations name the same
// file the same way, so the two logs diff.

#include <gr2/granny.h>

#include <cstddef>
#include <cstdint>
#include <string>

namespace NGr2
{

struct SFileInfo;
struct SModel;
struct SSkeleton;

//! FNV-1a over the whole file image, 64 bits.
//!
//! Not a cryptographic digest: this identifies a resource in a log, it does not
//! authenticate one. Ten dependency-free lines are worth more here than strength
//! would be, and 64 bits over a corpus of 21,720 distinct files is a collision
//! chance of about one in three thousand million. scripts/port/gr2whois.py
//! computes the same function, and the two have to stay in step.
uint64_t HashBytes( const void *pMemory, size_t nBytes );

//! Remember the bytes a file handle was read from.
//!
//! Called from the read entry points, where the buffer is still alive. Nothing
//! is logged yet: a file is worth describing once its objects exist, which is
//! GrannyGetFileInfo, and a file whose info is never asked for is one the caller
//! did nothing with.
void NoteFileBytes( const void *pFileHandle, const void *pMemory, size_t nBytes );

//! Remember the path a file handle was read from, for the by-name entry point.
//!
//! GrannyReadEntireFile is the editor's and SceneB2/TerraTools.cpp's route, never
//! the game's. The path is better identity than a hash would be there, so it is
//! recorded instead of one rather than as well as.
void NoteFileName( const void *pFileHandle, const char *pszFileName );

//! Register a file's objects and log one line describing it.
//!
//! Idempotent: the engine asks for a file's info more than once and only the
//! first call registers or logs. Null info means the conversion failed, which is
//! worth a line of its own since that file's objects will never appear again.
void NoteFileInfo( const void *pFileHandle, const SFileInfo *pInfo );

//! Drop a file's registration, on GrannyFreeFile.
//!
//! Without this a later file whose arena lands on the freed one's addresses
//! would be described as the dead one, which is worse than not knowing.
void ForgetFile( const void *pFileHandle );

//! How a skeleton should read in a message: its name, its bones, and its file.
//!
//! For example
//!
//!     skeleton "Hip" of 21 bones, from file#3 8f0c1d2e3a4b5c6d
//!     "J:/Complete/Units/Infantry/Animations/RIFLE/2.mb"
//!
//! and, for a pointer no file claims, a plain statement that nothing does. That
//! case is not a defect on its own: the engine builds a granny_model of its own
//! in CSkeletonAnimator and only borrows the skeleton inside it.
std::string DescribeSkeleton( const SSkeleton *pSkeleton );

//! The same for a model, by way of the skeleton it carries.
//!
//! The model itself is usually the engine's own stack object, so the skeleton is
//! what leads back to a file.
std::string DescribeModel( const SModel *pModel );

}
