#include <new>
#include <string>
#include <vector>

namespace
{
// Exercise allocation during DLL initialization, before the executable's main.
const std::string startup(512, 's');
}

extern "C"
{
__declspec(dllexport) const void* ModuleStartup() { return startup.data(); }
__declspec(dllexport) void* ModuleNew(std::size_t n) { return ::operator new(n); }
__declspec(dllexport) void ModuleDelete(void* p) { ::operator delete(p); }
__declspec(dllexport) void* ModuleNewAligned(std::size_t n)
{
    return ::operator new(n, std::align_val_t(256));
}
__declspec(dllexport) void ModuleDeleteAligned(void* p)
{
    ::operator delete(p, std::align_val_t(256));
}
__declspec(dllexport) void ModuleFill(std::string* text, std::vector<int>* values)
{
    // Both buffers start in the EXE, are freed/replaced here, then freed there.
    text->append(4096, 'b');
    values->resize(4096, 42);
}
}
