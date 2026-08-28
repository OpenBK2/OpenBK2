"""Inspect a Granny .gr2 file: header, sections, type tree and objects.

Three views, in increasing order of what they need.

  header   the magic block, the file header and the section table. Read straight
           out of the bytes, so it needs nothing but Python and works on a file
           whose codec nobody has implemented.

  types    the type tree, which is how a GR2 describes its own structures. Needs
           the sections decompressed, and decompression needs a granny2.dll.

  objects  the root object walked through that type tree: models, skeletons,
           bones, meshes, animations, and the strings and arrays they point at.

Only decompression uses the DLL. Everything above is Python reading the file,
which is the point: the `objects` output is what the file *contains*, not what
some implementation says it contains, so it is the answer libgr2's
GrannyGetFileInfo has to reproduce rather than a second opinion about it.

    python gr2info.py header  <file.gr2>
    python gr2info.py types   <file.gr2> [--name granny_file_info]
    python gr2info.py objects <file.gr2> [--depth 3]
    python gr2info.py <anything> --json          machine readable
    python gr2info.py <anything> --dll <path>    which granny2 decompresses

A .pak is an ordinary ZIP archive, so getting a file to point this at is:

    python -c "import zipfile; open('a.gr2','wb').write(
        zipfile.ZipFile(r'C:\\Games\\bk2\\Data\\data.pak').read(
            'bin/Animations/0001E9F6-C549-4E69-9497-5EA0CEB1993F'))"

See docs/GrannyReplacement.md for the format, and granny_dll_oracle.py for the
other direction, driving the real DLL's runtime rather than reading files.
"""

import argparse
import ctypes as C
import json
import os
import struct
import sys
from ctypes import c_bool, c_int32, c_void_p

# --- the format ------------------------------------------------------------
#
# Every constant here was measured over the shipped corpus rather than taken
# from a specification. Versions/Temporary/Engine/Sources/vendor/libgr2 carries
# the same numbers with the census behind each one.

MAGIC = bytes.fromhex('b867b0caf86db10f84728c7e5e19001e')
MAGIC_BLOCK_SIZE = 32
HEADER_OFFSET = 32
HEADER_SIZE = 56
SECTION_RECORD_SIZE = 44
POINTER_FIXUP_SIZE = 12
MIXED_MARSHALLING_FIXUP_SIZE = 16

# granny_compression_type. Read the numbers twice: Oodle0 is 1, because the enum
# starts at no compression.
COMPRESSION = {0: 'none', 1: 'Oodle0', 2: 'Oodle1', 3: 'BitKnit', 4: 'BitKnit2'}

# The four struct tags in the wild. 0x80000011 is three files of mod content.
TYPE_TAGS = {0x8000000f: '0x8000000f', 0x80000010: '0x80000010',
             0x80000011: '0x80000011 (mod content only)', 0x80000013: '0x80000013'}

# granny_member_type, from granny211.h. The order is the enum's, and the value is
# what a type definition's first word holds.
(END, INLINE, REFERENCE, REFERENCE_TO_ARRAY, ARRAY_OF_REFERENCES, VARIANT_REFERENCE,
 UNSUPPORTED_REMOVE, REFERENCE_TO_VARIANT_ARRAY, STRING, TRANSFORM, REAL32, INT8,
 UINT8, BINORMAL_INT8, NORMAL_UINT8, INT16, UINT16, BINORMAL_INT16, NORMAL_UINT16,
 INT32, UINT32, REAL16, EMPTY_REFERENCE) = range(23)

MEMBER_TYPE_NAMES = {
    END: 'End', INLINE: 'Inline', REFERENCE: 'Reference',
    REFERENCE_TO_ARRAY: 'ReferenceToArray', ARRAY_OF_REFERENCES: 'ArrayOfReferences',
    VARIANT_REFERENCE: 'VariantReference', UNSUPPORTED_REMOVE: 'Unsupported',
    REFERENCE_TO_VARIANT_ARRAY: 'ReferenceToVariantArray', STRING: 'String',
    TRANSFORM: 'Transform', REAL32: 'Real32', INT8: 'Int8', UINT8: 'UInt8',
    BINORMAL_INT8: 'BinormalInt8', NORMAL_UINT8: 'NormalUInt8', INT16: 'Int16',
    UINT16: 'UInt16', BINORMAL_INT16: 'BinormalInt16', NORMAL_UINT16: 'NormalUInt16',
    INT32: 'Int32', UINT32: 'UInt32', REAL16: 'Real16',
    EMPTY_REFERENCE: 'EmptyReference',
}

# Pointers in these files are 32 bits whatever the host is, which is the single
# fact that shapes every size below.
POINTER_SIZE = 4

# A type definition on disk: Type, Name, ReferenceType, ArrayWidth, Extra[3],
# Ignored. 4 + 4 + 4 + 4 + 12 + 4.
TYPE_DEFINITION_SIZE = 32

# What one member occupies, before multiplying by ArrayWidth. Inline is absent
# because its size is its referenced type's, which takes a lookup.
_SCALAR_SIZES = {
    REFERENCE: POINTER_SIZE,
    REFERENCE_TO_ARRAY: 4 + POINTER_SIZE,
    ARRAY_OF_REFERENCES: 4 + POINTER_SIZE,
    VARIANT_REFERENCE: 2 * POINTER_SIZE,
    REFERENCE_TO_VARIANT_ARRAY: 2 * POINTER_SIZE + 4,
    STRING: POINTER_SIZE,
    # Flags, Position[3], Orientation[4], ScaleShear[3][3].
    TRANSFORM: 4 + 12 + 16 + 36,
    REAL32: 4,
    INT8: 1, UINT8: 1, BINORMAL_INT8: 1, NORMAL_UINT8: 1,
    INT16: 2, UINT16: 2, BINORMAL_INT16: 2, NORMAL_UINT16: 2,
    INT32: 4, UINT32: 4,
    REAL16: 2,
    EMPTY_REFERENCE: POINTER_SIZE,
}


class Gr2Error(Exception):
    pass


class Reference(object):
    """A place in the file: which section, and how far into it."""

    __slots__ = ('section', 'offset')

    def __init__(self, section, offset):
        self.section = section
        self.offset = offset

    def __repr__(self):
        return '%d:%d' % (self.section, self.offset)

    def __eq__(self, other):
        return (self.section, self.offset) == (other.section, other.offset)

    def __hash__(self):
        return hash((self.section, self.offset))


class Gr2File(object):
    """A parsed .gr2: header, sections, and the fixups between them."""

    def __init__(self, data):
        self.raw = data
        if len(data) < MAGIC_BLOCK_SIZE + HEADER_SIZE or data[:16] != MAGIC:
            raise Gr2Error('not a GR2 (File Format 6, little endian, 32-bit pointers)')

        self.header_size, self.header_format = struct.unpack_from('<2I', data, 16)
        (self.version, self.total_size, self.crc, self.section_array_offset,
         self.section_count, root_type_section, root_type_offset,
         root_section, root_offset, self.type_tag) = struct.unpack_from(
            '<10I', data, HEADER_OFFSET)
        self.extra_tags = struct.unpack_from('<4I', data, HEADER_OFFSET + 40)

        if self.version != 6:
            raise Gr2Error('file format %d, expected 6' % self.version)
        if self.header_format != 0:
            raise Gr2Error('headerFormat %d, the header itself is compressed'
                           % self.header_format)

        self.root_object_type = Reference(root_type_section, root_type_offset)
        self.root_object = Reference(root_section, root_offset)

        base = HEADER_OFFSET + self.section_array_offset
        if base + SECTION_RECORD_SIZE * self.section_count > len(data):
            raise Gr2Error('the section array runs past the end of the file')

        self.sections = []
        for i in range(self.section_count):
            fields = struct.unpack_from('<11I', data, base + SECTION_RECORD_SIZE * i)
            self.sections.append(dict(zip(
                ('compression', 'data_offset', 'data_size', 'expanded_size',
                 'alignment', 'first_16bit', 'first_8bit', 'fixup_offset',
                 'fixup_count', 'marshalling_offset', 'marshalling_count'), fields)))

        self.expanded = None   # section index -> bytes, once decompressed
        self.fixups = None     # Reference -> Reference

    # --- decompression, the one part that needs somebody else's code -------

    def decompress(self, dll):
        """Expand every section, and read the pointer fixups.

        `dll` is a loaded granny2 with GrannyDecompressData. Nothing else here
        needs it, and a file whose sections are all uncompressed needs it not at
        all.
        """
        self.expanded = []
        for i, s in enumerate(self.sections):
            if s['expanded_size'] == 0:
                self.expanded.append(b'')
                continue

            start = s['data_offset']
            packed = self.raw[start:start + s['data_size']]
            if s['compression'] == 0:
                self.expanded.append(packed)
                continue
            if dll is None:
                raise Gr2Error('section %d is %s compressed and no DLL was given'
                               % (i, COMPRESSION.get(s['compression'], '?')))

            # The decoder reads past the compressed length, which is what
            # GrannyGetCompressedBytesPaddingSize is warning about.
            pad = dll.GrannyGetCompressedBytesPaddingSize(s['compression'])
            src = C.create_string_buffer(packed + b'\0' * pad, len(packed) + pad)
            dst = C.create_string_buffer(s['expanded_size'])
            ok = dll.GrannyDecompressData(
                s['compression'], False, len(packed), C.cast(src, c_void_p),
                s['first_16bit'], s['first_8bit'], s['expanded_size'],
                C.cast(dst, c_void_p))
            if not ok:
                raise Gr2Error('GrannyDecompressData refused section %d' % i)
            self.expanded.append(dst.raw[:s['expanded_size']])

        self.fixups = {}
        for i, s in enumerate(self.sections):
            for k in range(s['fixup_count']):
                at = s['fixup_offset'] + POINTER_FIXUP_SIZE * k
                frm, to_sec, to_off = struct.unpack_from('<3I', self.raw, at)
                self.fixups[Reference(i, frm)] = Reference(to_sec, to_off)

    # --- reading -----------------------------------------------------------

    def _bytes(self, ref, count):
        data = self.expanded[ref.section]
        if ref.offset + count > len(data):
            raise Gr2Error('read of %d at %r runs past the section' % (count, ref))
        return data[ref.offset:ref.offset + count]

    def u32(self, ref):
        return struct.unpack('<I', self._bytes(ref, 4))[0]

    def i32(self, ref):
        return struct.unpack('<i', self._bytes(ref, 4))[0]

    def f32(self, ref):
        return struct.unpack('<f', self._bytes(ref, 4))[0]

    def pointer(self, ref):
        """What the four byte slot at `ref` points at, or None.

        The slot's own contents mean nothing: the fixup array is what says which
        words are pointers and where they lead, so an unfixed slot is a null
        pointer however it happens to read.
        """
        return self.fixups.get(ref)

    def string(self, ref):
        """The NUL terminated string a pointer slot leads to, or None."""
        target = self.pointer(ref)
        if target is None:
            return None
        data = self.expanded[target.section]
        end = data.find(b'\0', target.offset)
        if end < 0:
            raise Gr2Error('unterminated string at %r' % target)
        return data[target.offset:end].decode('utf-8', 'replace')

    # --- the type tree -----------------------------------------------------

    def type_members(self, ref):
        """The members of the type definition at `ref`, up to its End marker.

        A GR2 describes its own structures, which is why a reader resolves
        members by name through this rather than by hardcoded offset. It is why
        one parser reads all four struct tags and another, written against fixed
        offsets for a fifth, silently produces garbage.
        """
        members = []
        at = Reference(ref.section, ref.offset)
        while True:
            kind = self.u32(at)
            if kind == END:
                break
            if kind >= len(MEMBER_TYPE_NAMES):
                raise Gr2Error('member type %d at %r is not one Granny defines'
                               % (kind, at))

            name = self.string(Reference(at.section, at.offset + 4))
            reference_type = self.pointer(Reference(at.section, at.offset + 8))
            array_width, e0, e1, e2 = struct.unpack(
                '<4i', self._bytes(Reference(at.section, at.offset + 12), 16))

            members.append({
                'name': name,
                'type': kind,
                'type_name': MEMBER_TYPE_NAMES[kind],
                'reference_type': reference_type,
                'array_width': array_width,
                'extra': [e0, e1, e2],
            })
            at = Reference(at.section, at.offset + TYPE_DEFINITION_SIZE)
            if len(members) > 4096:
                raise Gr2Error('type definition at %r has no End marker' % ref)
        return members

    def member_size(self, member):
        """What one member occupies, which is GrannyGetMemberTypeSize's job."""
        width = member['array_width'] if member['array_width'] > 0 else 1
        if member['type'] == INLINE:
            if member['reference_type'] is None:
                raise Gr2Error('inline member %r with no type' % member['name'])
            return self.object_size(member['reference_type']) * width
        try:
            return _SCALAR_SIZES[member['type']] * width
        except KeyError:
            raise Gr2Error('no size for member type %s' % member['type_name'])

    def object_size(self, type_ref):
        """The whole of an object of this type, which is GrannyGetTotalObjectSize."""
        return sum(self.member_size(m) for m in self.type_members(type_ref))

    # --- objects -----------------------------------------------------------

    def read_object(self, type_ref, ref, depth, seen=None):
        """Walk an object of the given type into plain Python.

        Follows references while `depth` allows, and stops at a type it has
        already entered, since a skeleton's bones point back at things that
        point at bones.
        """
        seen = set() if seen is None else seen
        out = {}
        at = ref.offset
        for member in self.type_members(type_ref):
            here = Reference(ref.section, at)
            out[member['name']] = self._read_member(member, here, depth, seen)
            at += self.member_size(member)
        return out

    def _read_member(self, member, at, depth, seen):
        kind = member['type']
        width = member['array_width'] if member['array_width'] > 0 else 1

        if kind == STRING:
            return self.string(at)

        if kind in (REAL32, INT32, UINT32, INT16, UINT16, INT8, UINT8, REAL16,
                    BINORMAL_INT8, NORMAL_UINT8, BINORMAL_INT16, NORMAL_UINT16):
            return self._read_scalars(kind, at, width)

        if kind == TRANSFORM:
            flags = self.u32(at)
            floats = struct.unpack('<16f', self._bytes(
                Reference(at.section, at.offset + 4), 64))
            return {'Flags': flags, 'Position': list(floats[0:3]),
                    'Orientation': list(floats[3:7]),
                    'ScaleShear': [list(floats[7:10]), list(floats[10:13]),
                                   list(floats[13:16])]}

        if kind == INLINE:
            if depth <= 0:
                return '<inline, deeper than --depth>'
            return self.read_object(member['reference_type'], at, depth - 1, seen)

        if kind in (REFERENCE, EMPTY_REFERENCE):
            target = self.pointer(at)
            if target is None:
                return None
            if depth <= 0:
                return '<%r, deeper than --depth>' % target
            return self.read_object(member['reference_type'], target, depth - 1, seen)

        if kind in (REFERENCE_TO_ARRAY, ARRAY_OF_REFERENCES):
            count = self.i32(at)
            target = self.pointer(Reference(at.section, at.offset + 4))
            if target is None or count <= 0:
                return []
            if depth <= 0:
                return '<%d entries at %r, deeper than --depth>' % (count, target)
            return self._read_array(kind, member, count, target, depth, seen)

        if kind == VARIANT_REFERENCE:
            variant_type = self.pointer(at)
            variant_object = self.pointer(Reference(at.section, at.offset + 4))
            if variant_type is None or variant_object is None:
                return None
            if depth <= 0:
                return '<variant at %r, deeper than --depth>' % variant_object
            return self.read_object(variant_type, variant_object, depth - 1, seen)

        if kind == REFERENCE_TO_VARIANT_ARRAY:
            variant_type = self.pointer(at)
            count = self.i32(Reference(at.section, at.offset + 4))
            target = self.pointer(Reference(at.section, at.offset + 8))
            if variant_type is None or target is None or count <= 0:
                return []
            if depth <= 0:
                return '<%d variants at %r, deeper than --depth>' % (count, target)
            stride = self.object_size(variant_type)
            return [self.read_object(variant_type,
                                     Reference(target.section, target.offset + i * stride),
                                     depth - 1, seen)
                    for i in range(min(count, 4096))]

        return '<%s not read>' % member['type_name']

    def _read_array(self, kind, member, count, target, depth, seen):
        # A cap, because a corrupt count is a hang otherwise and no shipped file
        # comes close to it.
        count = min(count, 65536)
        if kind == ARRAY_OF_REFERENCES:
            out = []
            for i in range(count):
                slot = Reference(target.section, target.offset + i * POINTER_SIZE)
                element = self.pointer(slot)
                out.append(None if element is None
                           else self.read_object(member['reference_type'], element,
                                                 depth - 1, seen))
            return out

        stride = self.object_size(member['reference_type'])
        return [self.read_object(member['reference_type'],
                                 Reference(target.section, target.offset + i * stride),
                                 depth - 1, seen)
                for i in range(count)]

    def _read_scalars(self, kind, at, width):
        fmt, size = {
            REAL32: ('f', 4), INT32: ('i', 4), UINT32: ('I', 4),
            INT16: ('h', 2), UINT16: ('H', 2),
            BINORMAL_INT16: ('h', 2), NORMAL_UINT16: ('H', 2),
            INT8: ('b', 1), UINT8: ('B', 1),
            BINORMAL_INT8: ('b', 1), NORMAL_UINT8: ('B', 1),
            REAL16: ('H', 2),
        }[kind]
        values = list(struct.unpack('<%d%s' % (width, fmt),
                                    self._bytes(at, size * width)))
        return values[0] if width == 1 else values


# --- the DLL, for decompression only ---------------------------------------

DEFAULT_DLL = os.path.join(
    os.path.dirname(os.path.abspath(__file__)), '..', '..',
    'third_party', 'uesp-esoapps', 'common', 'granny', 'win64', 'granny2_x64.dll')


def load_dll(path):
    """Bind the two entry points decompression needs, and nothing else.

    Deliberately minimal so that this keeps working against a granny2 that
    implements only part of the API, which is what libgr2 is for most of its
    life.
    """
    dll = C.CDLL(os.path.abspath(path))
    dll.GrannyGetCompressedBytesPaddingSize.argtypes = [c_int32]
    dll.GrannyGetCompressedBytesPaddingSize.restype = c_int32
    dll.GrannyDecompressData.argtypes = [c_int32, c_bool, c_int32, c_void_p,
                                         c_int32, c_int32, c_int32, c_void_p]
    dll.GrannyDecompressData.restype = c_bool
    return dll


# --- views -----------------------------------------------------------------

def view_header(gr2):
    sections = []
    for i, s in enumerate(gr2.sections):
        entry = dict(s)
        entry['index'] = i
        entry['codec'] = COMPRESSION.get(s['compression'], 'unknown %d' % s['compression'])
        sections.append(entry)

    return {
        'magic': MAGIC.hex(),
        'headerSize': gr2.header_size,
        'headerFormat': gr2.header_format,
        'version': gr2.version,
        'totalSize': gr2.total_size,
        'actualSize': len(gr2.raw),
        'crc': '0x%08x' % gr2.crc,
        'sectionArrayOffset': gr2.section_array_offset,
        'sectionCount': gr2.section_count,
        'typeTag': TYPE_TAGS.get(gr2.type_tag, '0x%08x (unknown)' % gr2.type_tag),
        'rootObject': repr(gr2.root_object),
        'rootObjectType': repr(gr2.root_object_type),
        'extraTags': ['0x%08x' % t for t in gr2.extra_tags],
        'sections': sections,
    }


def print_header(info):
    print('%-22s %s' % ('magic', info['magic']))
    for key in ('version', 'headerFormat', 'headerSize', 'totalSize', 'actualSize',
                'crc', 'typeTag', 'rootObject', 'rootObjectType'):
        print('%-22s %s' % (key, info[key]))
    if info['totalSize'] != info['actualSize']:
        print('%-22s the header and the file disagree about the length' % 'WARNING')

    print('\nsections')
    print('  %2s %-8s %9s %9s %9s %6s %9s %8s %6s'
          % ('#', 'codec', 'offset', 'packed', 'expanded', 'align',
             'first16/8', 'fixups', 'marsh'))
    for s in info['sections']:
        stops = ('%d/%d' % (s['first_16bit'], s['first_8bit'])
                 if s['expanded_size'] else '-')
        print('  %2d %-8s %9d %9d %9d %6d %9s %8d %6d'
              % (s['index'], s['codec'], s['data_offset'], s['data_size'],
                 s['expanded_size'], s['alignment'], stops,
                 s['fixup_count'], s['marshalling_count']))

    packed = sum(s['data_size'] for s in info['sections'])
    expanded = sum(s['expanded_size'] for s in info['sections'])
    ratio = (' (%.1fx)' % (float(expanded) / packed)) if packed else ''
    print('\n  %d bytes packed, %d expanded%s, %d pointer fixups'
          % (packed, expanded, ratio, sum(s['fixup_count'] for s in info['sections'])))


def view_types(gr2, name_filter):
    """Every type definition reachable from the root object's type."""
    out = []
    pending = [gr2.root_object_type]
    seen = set()
    while pending:
        ref = pending.pop(0)
        if ref in seen:
            continue
        seen.add(ref)

        members = gr2.type_members(ref)
        entry = {'at': repr(ref), 'size': gr2.object_size(ref), 'members': []}
        for m in members:
            entry['members'].append({
                'name': m['name'],
                'type': m['type_name'],
                'arrayWidth': m['array_width'],
                'size': gr2.member_size(m),
                'referenceType': repr(m['reference_type']) if m['reference_type'] else None,
            })
            if m['reference_type'] is not None:
                pending.append(m['reference_type'])
        out.append(entry)

    if name_filter:
        out = [e for e in out
               if any(name_filter in (m['name'] or '') for m in e['members'])]
    return out


def print_types(types):
    for entry in types:
        print('type at %s, %d bytes' % (entry['at'], entry['size']))
        offset = 0
        for m in entry['members']:
            print('  +%-4d %-26s %-22s %4d%s'
                  % (offset, m['name'], m['type'], m['size'],
                     '' if m['arrayWidth'] in (0, 1) else '  [%d]' % m['arrayWidth']))
            offset += m['size']
        print('')
    print('%d types reachable from the root object' % len(types))


def summarise(obj):
    """One line for a walked object, when it is one of the ones we know."""
    if not isinstance(obj, dict):
        return None
    for key in ('Name', 'FromFileName'):
        if isinstance(obj.get(key), str):
            return obj[key]
    return None


def print_object(obj, indent=0, key=None):
    pad = '  ' * indent
    label = ('%s: ' % key) if key else ''

    if isinstance(obj, dict):
        name = summarise(obj)
        print('%s%s{%s' % (pad, label, (' %s' % name) if name else ''))
        for k, v in obj.items():
            if k in ('Name', 'FromFileName') and v == name:
                continue
            print_object(v, indent + 1, k)
        print('%s}' % pad)
    elif isinstance(obj, list):
        if not obj:
            print('%s%s[]' % (pad, label))
        elif all(not isinstance(x, (dict, list)) for x in obj):
            shown = obj if len(obj) <= 8 else obj[:8]
            more = '' if len(obj) <= 8 else ', ... %d total' % len(obj)
            print('%s%s[%s%s]' % (pad, label, ', '.join(_fmt(x) for x in shown), more))
        else:
            print('%s%s[%d]' % (pad, label, len(obj)))
            for i, x in enumerate(obj):
                print_object(x, indent + 1, '#%d' % i)
    else:
        print('%s%s%s' % (pad, label, _fmt(obj)))


def _fmt(value):
    if isinstance(value, float):
        return '%.6g' % value
    return str(value)


def main(argv):
    parser = argparse.ArgumentParser(
        description='Inspect a Granny .gr2 file.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__.split('\n\n', 1)[1])
    parser.add_argument('view', choices=('header', 'types', 'objects'))
    parser.add_argument('file')
    parser.add_argument('--json', action='store_true', help='machine readable output')
    parser.add_argument('--dll', default=DEFAULT_DLL,
                        help='the granny2 that decompresses; header needs none')
    parser.add_argument('--depth', type=int, default=3,
                        help='how far objects follows references (default 3)')
    parser.add_argument('--name', default=None,
                        help='types: only those with a member whose name contains this')
    args = parser.parse_args(argv[1:])

    with open(args.file, 'rb') as f:
        gr2 = Gr2File(f.read())

    if args.view == 'header':
        info = view_header(gr2)
        print(json.dumps(info, indent=2)) if args.json else print_header(info)
        return 0

    needs_dll = any(s['compression'] != 0 and s['expanded_size']
                    for s in gr2.sections)
    dll = load_dll(args.dll) if needs_dll else None
    gr2.decompress(dll)

    if args.view == 'types':
        types = view_types(gr2, args.name)
        print(json.dumps(types, indent=2)) if args.json else print_types(types)
        return 0

    root = gr2.read_object(gr2.root_object_type, gr2.root_object, args.depth)
    print(json.dumps(root, indent=2, default=str)) if args.json else print_object(root)
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main(sys.argv))
    except Gr2Error as e:
        sys.stderr.write('gr2info: %s\n' % e)
        sys.exit(1)
