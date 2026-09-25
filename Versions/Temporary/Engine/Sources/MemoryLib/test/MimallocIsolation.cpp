#include <gtest/gtest.h>
#include <mimalloc.h>
#include <objbase.h>

#include <cstdint>
#include <cstdlib>
#include <new>
#include <limits>
#include <string>
#include <thread>
#include <vector>

extern "C"
{
__declspec(dllimport) const void* ModuleStartup();
__declspec(dllimport) void* ModuleNew(std::size_t n);
__declspec(dllimport) void ModuleDelete(void* p);
__declspec(dllimport) void* ModuleNewAligned(std::size_t n);
__declspec(dllimport) void ModuleDeleteAligned(void* p);
__declspec(dllimport) void ModuleFill(std::string* text, std::vector<int>* values);
__declspec(dllimport) void* ForeignNew(std::size_t n);
__declspec(dllimport) void ForeignDelete(void* p);
}

TEST(MimallocIsolation, CrossModuleAndThreadOwnership)
{
    EXPECT_TRUE(mi_is_in_heap_region(ModuleStartup()));
    void* fromDll = ModuleNew(1536);
    EXPECT_TRUE(mi_is_in_heap_region(fromDll));
    ::operator delete(fromDll);

    void* fromExe = ::operator new(1536);
    EXPECT_TRUE(mi_is_in_heap_region(fromExe));
    // Freeing on a different thread must still reach the same allocator.
    std::thread worker([fromExe] { ModuleDelete(fromExe); });
    worker.join();

    std::string text(512, 'a');
    std::vector<int> values(512, 7);
    ModuleFill(&text, &values);
    EXPECT_TRUE(mi_is_in_heap_region(text.data()));
    EXPECT_TRUE(mi_is_in_heap_region(values.data()));
    EXPECT_EQ(text.size(), 4608u);
    EXPECT_EQ(values.back(), 42);
}

TEST(MimallocIsolation, ArrayNothrowSizedAndAlignedOverloads)
{
    void* array = ::operator new[](1536, std::nothrow);
    ASSERT_NE(array, nullptr);
    EXPECT_TRUE(mi_is_in_heap_region(array));
    ::operator delete[](array, std::nothrow);

    void* scalar = ::operator new(1536, std::nothrow);
    ASSERT_NE(scalar, nullptr);
    EXPECT_TRUE(mi_is_in_heap_region(scalar));
    ::operator delete(scalar, std::size_t(1536));

    void* fromDll = ModuleNewAligned(1536);
    EXPECT_TRUE(mi_is_in_heap_region(fromDll));
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(fromDll) % 256, 0u);
    ::operator delete(fromDll, std::align_val_t(256));

    void* fromExe = ::operator new(1536, std::align_val_t(256), std::nothrow);
    ASSERT_NE(fromExe, nullptr);
    EXPECT_TRUE(mi_is_in_heap_region(fromExe));
    ModuleDeleteAligned(fromExe);

    void* alignedArray = ::operator new[](1536, std::align_val_t(256));
    EXPECT_TRUE(mi_is_in_heap_region(alignedArray));
    ::operator delete[](alignedArray, std::size_t(1536), std::align_val_t(256));
}

TEST(MimallocIsolation, AllocationFailureUsesCppSemantics)
{
    // Volatile prevents compile-time oversized-allocation diagnostics.
    volatile std::size_t impossible = (std::numeric_limits<std::size_t>::max)();
    EXPECT_THROW((void)::operator new(impossible), std::bad_alloc);
    EXPECT_EQ(::operator new(impossible, std::nothrow), nullptr);
}

TEST(MimallocIsolation, CrtComAndUnmodifiedDllKeepTheirAllocators)
{
    void* crt = std::malloc(1536);
    ASSERT_NE(crt, nullptr);
    EXPECT_FALSE(mi_is_in_heap_region(crt));
    std::free(crt);

    void* com = CoTaskMemAlloc(1536);
    ASSERT_NE(com, nullptr);
    EXPECT_FALSE(mi_is_in_heap_region(com));
    CoTaskMemFree(com);

    void* foreign = ForeignNew(1536);
    ASSERT_NE(foreign, nullptr);
    EXPECT_FALSE(mi_is_in_heap_region(foreign));
    ForeignDelete(foreign);
}
