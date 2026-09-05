"""Read dialog templates out of a binary, and write them back out as .rc text.

The same trick legacy.rc came from: the shipped 2005 editor has the resources
this port is missing compiled into it, and a dialog template is a documented
structure, so it can be read back and turned into the DIALOGEX block that would
have produced it.

    python resdlg.py "C:\\Games\\BK2-FoTR\\bin_\\B2_MapEditor.exe" --list
    python resdlg.py <binary> --list --grep customize
    python resdlg.py <binary> --dump 30001

Nothing is copied that is not a description of a layout: sizes, styles and
control ids. Strings that are part of the template come with it, which is what a
caption is.
"""
import argparse
import ctypes as C
import struct
from ctypes import wintypes

k = C.WinDLL('kernel32', use_last_error=True)
k.LoadLibraryExW.restype = wintypes.HMODULE
k.LoadLibraryExW.argtypes = [wintypes.LPCWSTR, wintypes.HANDLE, wintypes.DWORD]
k.FindResourceW.restype = wintypes.HANDLE
k.FindResourceW.argtypes = [wintypes.HMODULE, C.c_void_p, C.c_void_p]
# argtypes matter as much as restype here: a resource handle in a 64-bit
# process does not fit in the int ctypes assumes for an unprototyped argument,
# and the call dies with "int too long to convert".
k.LoadResource.restype = wintypes.HANDLE
k.LoadResource.argtypes = [wintypes.HMODULE, wintypes.HANDLE]
k.SizeofResource.restype = wintypes.DWORD
k.SizeofResource.argtypes = [wintypes.HMODULE, wintypes.HANDLE]
k.LockResource.restype = C.c_void_p
k.LockResource.argtypes = [wintypes.HANDLE]

LOAD_LIBRARY_AS_DATAFILE = 0x00000002
RT_DIALOG = 5
RT_STRING = 6

# The name is c_void_p, not LPCWSTR, on purpose: a resource name is either a
# pointer to a wide string or a small integer standing for itself, and ctypes
# told it is a string will try to read one out of the integer.
ENUMRESNAMEPROC = C.WINFUNCTYPE(wintypes.BOOL, wintypes.HMODULE, C.c_void_p,
                                C.c_void_p, wintypes.LPARAM)
k.EnumResourceNamesW.argtypes = [wintypes.HMODULE, C.c_void_p,
                                 ENUMRESNAMEPROC, wintypes.LPARAM]


def as_res(value):
    """MAKEINTRESOURCE, or a real string pointer."""
    if isinstance(value, int):
        return C.c_void_p(value)
    return C.c_void_p(C.cast(C.c_wchar_p(value), C.c_void_p).value)


def name_of(ptr):
    """A resource name back from the pointer the enumerator handed over."""
    if ptr is None:
        return 0
    if ptr < 0x10000:
        return int(ptr)
    return C.wstring_at(ptr)

# The window classes a template can name by number.
ORD_CLASS = {0x80: 'BUTTON', 0x81: 'EDIT', 0x82: 'STATIC', 0x83: 'LISTBOX',
             0x84: 'SCROLLBAR', 0x85: 'COMBOBOX'}


class Reader(object):
    def __init__(self, data):
        self.d = data
        self.p = 0

    def u16(self):
        v = struct.unpack_from('<H', self.d, self.p)[0]
        self.p += 2
        return v

    def i16(self):
        v = struct.unpack_from('<h', self.d, self.p)[0]
        self.p += 2
        return v

    def u32(self):
        v = struct.unpack_from('<I', self.d, self.p)[0]
        self.p += 4
        return v

    def align(self, n=4):
        self.p = (self.p + n - 1) & ~(n - 1)

    def sz_or_ord(self):
        """Either 0 (nothing), 0xFFFF plus an ordinal, or a wide string."""
        first = self.u16()
        if first == 0:
            return ''
        if first == 0xFFFF:
            return self.u16()
        out = [first]
        while True:
            c = self.u16()
            if c == 0:
                break
            out.append(c)
        return ''.join(chr(c) for c in out)


def parse(data):
    r = Reader(data)
    ex = struct.unpack_from('<HH', data, 0) == (1, 0xFFFF)
    dlg = {'ex': ex, 'items': []}
    if ex:
        r.u16(); r.u16()          # dlgVer, signature
        r.u32()                   # helpID
        dlg['exstyle'] = r.u32()
        dlg['style'] = r.u32()
        count = r.u16()
    else:
        dlg['style'] = r.u32()
        dlg['exstyle'] = r.u32()
        count = r.u16()
    dlg['x'], dlg['y'], dlg['cx'], dlg['cy'] = r.i16(), r.i16(), r.i16(), r.i16()
    dlg['menu'] = r.sz_or_ord()
    dlg['class'] = r.sz_or_ord()
    dlg['title'] = r.sz_or_ord()
    if dlg['style'] & 0x40:       # DS_SETFONT
        dlg['pointsize'] = r.u16()
        if ex:
            dlg['weight'] = r.u16()
            dlg['italic'] = r.u16() & 0xFF
        dlg['typeface'] = r.sz_or_ord()
    for _ in range(count):
        r.align(4)
        item = {}
        if ex:
            r.u32()               # helpID
            item['exstyle'] = r.u32()
            item['style'] = r.u32()
        else:
            item['style'] = r.u32()
            item['exstyle'] = r.u32()
        item['x'], item['y'], item['cx'], item['cy'] = r.i16(), r.i16(), r.i16(), r.i16()
        # DLGTEMPLATEEX widened the id to 32 bits, so IDC_STATIC reads back as
        # 0xFFFFFFFF there and 0xFFFF in the old form. It is the same id.
        item['id'] = (r.u32() & 0xFFFF) if ex else r.u16()
        item['class'] = r.sz_or_ord()
        item['title'] = r.sz_or_ord()
        extra = r.u16()
        r.p += extra
        dlg['items'].append(item)
    return dlg


def cls_name(v):
    return ORD_CLASS.get(v, v) if isinstance(v, int) else v


def to_rc(name, dlg):
    out = []
    kind = 'DIALOGEX' if dlg['ex'] else 'DIALOG'
    out.append('%s %s %d, %d, %d, %d' % (name, kind, dlg['x'], dlg['y'], dlg['cx'], dlg['cy']))
    out.append('STYLE 0x%08X' % dlg['style'])
    if dlg['exstyle']:
        out.append('EXSTYLE 0x%08X' % dlg['exstyle'])
    if dlg['title']:
        out.append('CAPTION "%s"' % dlg['title'])
    if 'typeface' in dlg:
        out.append('FONT %d, "%s"' % (dlg['pointsize'], dlg['typeface']))
    out.append('BEGIN')
    for it in dlg['items']:
        out.append('    CONTROL         "%s",%d,"%s",0x%08X,%d,%d,%d,%d%s'
                   % (it['title'] if isinstance(it['title'], str) else '',
                      it['id'], cls_name(it['class']), it['style'],
                      it['x'], it['y'], it['cx'], it['cy'],
                      (',0x%08X' % it['exstyle']) if it['exstyle'] else ''))
    out.append('END')
    return '\n'.join(out)


def strings(load, names):
    """Every string in the table, by id.

    RT_STRING is stored sixteen strings to a resource: bundle n holds ids
    16*(n-1) to 16*(n-1)+15, each a word of length followed by that many wide
    characters, and a length of zero means the id is unused.
    """
    out = {}
    for name in names:
        if not isinstance(name, int):
            continue
        raw = load(name, RT_STRING)
        if raw is None:
            continue
        r = Reader(raw)
        base = (name - 1) * 16
        for i in range(16):
            if r.p >= len(raw):
                break
            n = r.u16()
            if n:
                out[base + i] = ''.join(chr(r.u16()) for _ in range(n))
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('binary')
    ap.add_argument('--list', action='store_true')
    ap.add_argument('--grep', help='only dialogs whose caption or a control text matches')
    ap.add_argument('--dump', help='resource name or id to print as .rc')
    ap.add_argument('--strings', action='store_true',
                    help='list the string table instead, id and text')
    ap.add_argument('--range', help='with --strings, only ids in LO-HI')
    args = ap.parse_args()

    h = k.LoadLibraryExW(args.binary, None, LOAD_LIBRARY_AS_DATAFILE)
    if not h:
        raise SystemExit('LoadLibraryEx failed: %d' % C.get_last_error())

    def enum(rtype):
        found = []

        def cb(hmod, typ, name, lp):
            found.append(name_of(name))
            return True

        # Held in a variable: a callback that only exists as an argument can be
        # collected while the enumerator is still calling it.
        proc = ENUMRESNAMEPROC(cb)
        k.EnumResourceNamesW(h, as_res(rtype), proc, 0)
        return found

    def load(name, rtype=RT_DIALOG):
        res = k.FindResourceW(h, as_res(name), as_res(rtype))
        if not res:
            return None
        size = k.SizeofResource(h, res)
        ptr = k.LockResource(k.LoadResource(h, res))
        return C.string_at(ptr, size)

    if args.strings:
        table = strings(load, enum(RT_STRING))
        lo, hi = 0, 0xFFFF
        if args.range:
            lo, hi = (int(v) for v in args.range.split('-'))
        for sid in sorted(table):
            if lo <= sid <= hi:
                text = table[sid]
                if not args.grep or args.grep.lower() in text.lower():
                    print('  %-6d %s' % (sid, text))
        return 0

    names = enum(RT_DIALOG)
    print('%d dialog templates in %s' % (len(names), args.binary))
    for name in names:
        raw = load(name)
        if raw is None:
            continue
        try:
            dlg = parse(raw)
        except Exception as e:                                  # noqa: BLE001
            print('  %-8s unreadable (%s)' % (name, e))
            continue
        text = ' '.join([str(dlg['title'])] +
                        [str(i['title']) for i in dlg['items'] if isinstance(i['title'], str)])
        if args.grep and args.grep.lower() not in text.lower():
            continue
        if args.dump and str(name) != str(args.dump):
            continue
        if args.dump:
            print()
            print(to_rc(str(name), dlg))
        else:
            print('  %-8s %3d controls  %4dx%-4d  %s'
                  % (name, len(dlg['items']), dlg['cx'], dlg['cy'], dlg['title'] or ''))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
