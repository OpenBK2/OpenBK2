"""Read string resources out of a PE without running it.

LoadLibraryEx with LOAD_LIBRARY_AS_IMAGE_RESOURCE maps only the resource
section, so this reads a 32-bit binary's strings from a 64-bit interpreter and
never executes any of its code.

    python resstr.py C:\\path\\to\\Some.exe 140 141
"""
import ctypes as C
import sys
from ctypes import wintypes

k = C.WinDLL('kernel32', use_last_error=True)
u = C.WinDLL('user32', use_last_error=True)

k.LoadLibraryExW.argtypes = [wintypes.LPCWSTR, wintypes.HANDLE, wintypes.DWORD]
k.LoadLibraryExW.restype = wintypes.HMODULE
u.LoadStringW.argtypes = [wintypes.HMODULE, wintypes.UINT, wintypes.LPWSTR, C.c_int]

LOAD_LIBRARY_AS_IMAGE_RESOURCE = 0x00000020
LOAD_LIBRARY_AS_DATAFILE = 0x00000002


def main():
    path, ids = sys.argv[1], [int(a) for a in sys.argv[2:]]
    h = k.LoadLibraryExW(path, None,
                         LOAD_LIBRARY_AS_IMAGE_RESOURCE | LOAD_LIBRARY_AS_DATAFILE)
    if not h:
        sys.exit(f'LoadLibraryEx failed: {C.get_last_error()}')
    # The handle has its low bits tagged for a resource-only mapping; LoadString
    # wants it as given, so it is passed straight through.
    for nid in ids:
        buf = C.create_unicode_buffer(1024)
        n = u.LoadStringW(h, nid, buf, 1024)
        print(f'{nid}: len={n} {buf.value!r}' if n else f'{nid}: MISSING')


if __name__ == '__main__':
    main()
