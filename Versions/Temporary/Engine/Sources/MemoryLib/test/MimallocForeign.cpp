#include <new>

// No engine headers or allocator overrides: model an independently built DLL.
extern "C"
{
__declspec(dllexport) void* ForeignNew(std::size_t n) { return ::operator new(n); }
__declspec(dllexport) void ForeignDelete(void* p) { ::operator delete(p); }
}
