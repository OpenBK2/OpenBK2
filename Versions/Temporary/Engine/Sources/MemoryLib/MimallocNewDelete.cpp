// Linked once into each participating Windows EXE/DLL, without a project PCH.
// This covers scalar, array, nothrow, sized and C++17 aligned new/delete. C APIs
// (malloc/free, COM, Win32 and third-party buffer APIs) retain their own pairs.
// Do not statically link a private mimalloc heap into each engine DLL: objects
// cross DLL boundaries and must all reach the same shared allocator instance.
#include <mimalloc-new-delete.h>
