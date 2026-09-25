# mimalloc on Windows

With `USE_MIMALLOC=ON`, the engine, its executables and tests use module-local
C++ `new/delete` overrides. All of these call one shared `mimalloc.dll`.
The override object includes scalar, array, sized, nothrow and C++17 aligned
overloads from [mimalloc's C++ wrapper](https://microsoft.github.io/mimalloc/using.html),
and does not use an engine precompiled header. mimalloc is compiled as C++ so
allocation failure honors the new handler and throws `std::bad_alloc`.

`MI_OVERRIDE` and `MI_WIN_REDIRECT` are forced off on Windows. Neither the
process CRT nor Windows DLLs are patched. This avoids the fullscreen D3D9 gamma
ramp crash documented in `mimalloc_windows_issue/`: Windows colour management
allocates a buffer through CRT new and releases it through COM.

The bundled Boost, fastgltf, fmt, spdlog, GoogleTest, Google Benchmark and
wxWidgets C++ modules use the same pair because owned C++ objects can cross
those DLL boundaries. Static libraries use the overrides in their final
EXE/DLL. wxWidgets receives the shared override object through a CMake project
hook; its build cache identity changes when allocator mode changes.

C allocation is unchanged: pair `malloc/realloc/free`, `CoTaskMemAlloc/Free`,
Win32 heap functions and third-party buffer APIs with their own corresponding
functions. Do not give an independently built DLL ownership of an engine
`new` allocation unless its API invokes an engine-provided deleter. New
bundled C++ DLL dependencies that exchange ownership need to join the allocator
setup in `cmake/project_allocator.cmake`.

Reconfigure and rebuild the executable and its DLL dependencies together, then
deploy the rebuilt `mimalloc.dll` too. Reusing the old redirect-enabled DLL would
reintroduce the problem. The redirect DLL is no longer linked or installed.
`USE_MIMALLOC=OFF` and `ENABLE_ASAN=ON` remove the local overrides, including the
wxWidgets hook. The non-Windows allocator setup is unchanged.

From the Visual Studio Developer Environment, using Visual Studio's CMake:

```
cmake --build out/build/Windows-x64-Release --target MimallocIsolation_test
ctest --test-dir out/build/Windows-x64-Release -R "^MimallocIsolation_test$" --output-on-failure
```

The test uses two real DLLs: one participating module and one untouched CRT
module. It covers pre-main allocation, freeing across modules and threads, STL
buffer replacement across a DLL boundary, aligned/nothrow/sized allocation,
allocation failure semantics,
and verifies that CRT, COM and the untouched module stay outside mimalloc.
Fullscreen startup still needs checking on a display that exercises the
Advanced Color path described in the crash report.
