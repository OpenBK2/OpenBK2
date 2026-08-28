"""Compare two granny2 implementations over the whole corpus, file by file.

Loads both DLLs into one process, reads every .gr2 it can find through each,
walks what GrannyGetFileInfo returns, and reports where the two disagree. That
works because libgr2 reproduces the Granny API rather than inventing a neutral
one: the same ctypes bindings drive either, and the structures walked are
granny211.h's, which is the contract both have to meet.

    python gr2diff.py                        every install this knows about
    python gr2diff.py <dir-or-pak> ...       somewhere else
    python gr2diff.py --limit 500            a quick sample
    python gr2diff.py --jobs 8               spread over processes
    python gr2diff.py --report out.json      the differences, in full

By default the reference is the vendored granny2_x64.dll and the candidate is
whatever libgr2 last built into out/build/libgr2-x64; --reference and --candidate
override either. A 64-bit Python is required, since those are 64-bit DLLs.

Files the candidate refuses but the reference reads are counted separately from
files the two read differently, because during a port most of them are one
milestone away rather than wrong. Where the reason is a codec that is not
implemented yet, that is a "not supported" line and not a failure.

Output is a histogram before it is a list: over eighty thousand files, what is
useful is "InverseWorld4x4 differs in 12 files", not twelve thousand lines.
"""

import argparse
import ctypes as C
import fnmatch
import hashlib
import io
import json
import multiprocessing
import os
import struct
import sys
import zipfile
from collections import Counter
from ctypes import POINTER, c_char_p, c_float, c_int32, c_uint16, c_uint32, c_void_p

MAGIC = bytes.fromhex('b867b0caf86db10f84728c7e5e19001e')

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, '..', '..'))

DEFAULT_REFERENCE = os.path.join(REPO, 'third_party', 'uesp-esoapps', 'common',
                                 'granny', 'win64', 'granny2_x64.dll')
DEFAULT_CANDIDATE = os.path.join(REPO, 'out', 'build', 'libgr2-x64', 'granny2_x64.dll')

# The three installs the corpus was surveyed over. Missing ones are skipped, so
# this is a default rather than a requirement.
DEFAULT_CORPUS = [
    r'C:\Games\bk2',
    r'C:\Games\BK2-FoTR',
    r'C:\Games\Blitzkrieg 2 -  Total Conversion',
]

# Where GR2 resources live inside a pak, and what they are called outside one.
GR2_DIRECTORIES = ('/Geometries/', '/Animations/', '/Skeletons/', '/AIGeometries/')


# --- the structures both implementations have to produce --------------------
#
# granny211.h's, packed, which is what the engine compiles against. See
# Versions/Temporary/Engine/Sources/vendor/libgr2/src/Structures.h for the same
# layouts in C++ with their sizes asserted.

class Variant(C.Structure):
    _pack_ = 1
    _fields_ = [('Type', c_void_p), ('Object', c_void_p)]


class Transform(C.Structure):
    _pack_ = 1
    _fields_ = [('Flags', c_uint32), ('Position', c_float * 3),
                ('Orientation', c_float * 4), ('ScaleShear', (c_float * 3) * 3)]


class DataType(C.Structure):
    _pack_ = 1


DataType._fields_ = [('Type', c_int32), ('Name', c_char_p),
                     ('ReferenceType', POINTER(DataType)), ('ArrayWidth', c_int32),
                     ('Extra', c_int32 * 3), ('Ignored', c_void_p)]


class Bone(C.Structure):
    _pack_ = 1
    _fields_ = [('Name', c_char_p), ('ParentIndex', c_int32),
                ('LocalTransform', Transform), ('InverseWorld4x4', c_float * 16),
                ('LODError', c_float), ('ExtendedData', Variant)]


class Skeleton(C.Structure):
    _pack_ = 1
    _fields_ = [('Name', c_char_p), ('BoneCount', c_int32), ('Bones', POINTER(Bone)),
                ('LODType', c_int32), ('ExtendedData', Variant)]


class TriMaterialGroup(C.Structure):
    _pack_ = 1
    _fields_ = [('MaterialIndex', c_int32), ('TriFirst', c_int32), ('TriCount', c_int32)]


class TriTopology(C.Structure):
    _pack_ = 1
    _fields_ = [
        ('GroupCount', c_int32), ('Groups', POINTER(TriMaterialGroup)),
        ('IndexCount', c_int32), ('Indices', POINTER(c_int32)),
        ('Index16Count', c_int32), ('Indices16', POINTER(c_uint16)),
        ('VertexToVertexCount', c_int32), ('VertexToVertexMap', c_void_p),
        ('VertexToTriangleCount', c_int32), ('VertexToTriangleMap', c_void_p),
        ('SideToNeighborCount', c_int32), ('SideToNeighborMap', c_void_p),
        ('PolygonIndexStartCount', c_int32), ('PolygonIndexStarts', c_void_p),
        ('PolygonIndexCount', c_int32), ('PolygonIndices', c_void_p),
        ('BonesForTriangleCount', c_int32), ('BonesForTriangle', c_void_p),
        ('TriangleToBoneCount', c_int32), ('TriangleToBoneIndices', c_void_p),
        ('TriAnnotationSetCount', c_int32), ('TriAnnotationSets', c_void_p),
    ]


class VertexData(C.Structure):
    _pack_ = 1
    _fields_ = [('VertexType', POINTER(DataType)), ('VertexCount', c_int32),
                ('Vertices', POINTER(c_uint32)),
                ('VertexComponentNameCount', c_int32), ('VertexComponentNames', c_void_p),
                ('VertexAnnotationSetCount', c_int32), ('VertexAnnotationSets', c_void_p)]


class BoneBinding(C.Structure):
    _pack_ = 1
    _fields_ = [('BoneName', c_char_p), ('OBBMin', c_float * 3), ('OBBMax', c_float * 3),
                ('TriangleCount', c_int32), ('TriangleIndices', POINTER(c_int32))]


class Mesh(C.Structure):
    _pack_ = 1
    _fields_ = [
        ('Name', c_char_p), ('PrimaryVertexData', POINTER(VertexData)),
        ('MorphTargetCount', c_int32), ('MorphTargets', c_void_p),
        ('PrimaryTopology', POINTER(TriTopology)),
        ('MaterialBindingCount', c_int32), ('MaterialBindings', c_void_p),
        ('BoneBindingCount', c_int32), ('BoneBindings', POINTER(BoneBinding)),
        ('ExtendedData', Variant),
    ]


class ModelMeshBinding(C.Structure):
    _pack_ = 1
    _fields_ = [('Mesh', POINTER(Mesh))]


class Model(C.Structure):
    _pack_ = 1
    _fields_ = [('Name', c_char_p), ('Skeleton', POINTER(Skeleton)),
                ('InitialPlacement', Transform), ('MeshBindingCount', c_int32),
                ('MeshBindings', POINTER(ModelMeshBinding)), ('ExtendedData', Variant)]


class FileInfo(C.Structure):
    _pack_ = 1
    _fields_ = [
        ('ArtToolInfo', c_void_p), ('ExporterInfo', c_void_p), ('FromFileName', c_char_p),
        ('TextureCount', c_int32), ('Textures', c_void_p),
        ('MaterialCount', c_int32), ('Materials', c_void_p),
        ('SkeletonCount', c_int32), ('Skeletons', POINTER(POINTER(Skeleton))),
        ('VertexDataCount', c_int32), ('VertexDatas', c_void_p),
        ('TriTopologyCount', c_int32), ('TriTopologies', POINTER(POINTER(TriTopology))),
        ('MeshCount', c_int32), ('Meshes', POINTER(POINTER(Mesh))),
        ('ModelCount', c_int32), ('Models', POINTER(POINTER(Model))),
        ('TrackGroupCount', c_int32), ('TrackGroups', c_void_p),
        ('AnimationCount', c_int32), ('Animations', c_void_p),
        ('ExtendedData', Variant),
    ]


_SIZES = ((Variant, 16), (Transform, 68), (DataType, 44), (Bone, 164), (Skeleton, 40),
          (Model, 112), (FileInfo, 148), (Mesh, 76), (VertexData, 44),
          (TriTopology, 132), (BoneBinding, 44), (TriMaterialGroup, 12))
for _t, _n in _SIZES:
    assert C.sizeof(_t) == _n, '%s is %d, expected %d' % (_t.__name__, C.sizeof(_t), _n)


# --- driving one implementation --------------------------------------------

class Granny(object):
    """The five entry points this needs, bound out of one DLL.

    Deliberately no more than five, so that this keeps working against a granny2
    that implements only part of the API, which libgr2 does for most of its life.
    """

    def __init__(self, path):
        self.path = path
        self.dll = C.CDLL(os.path.abspath(path))
        self.dll.GrannyReadEntireFileFromMemory.argtypes = [c_int32, c_void_p]
        self.dll.GrannyReadEntireFileFromMemory.restype = c_void_p
        self.dll.GrannyGetFileInfo.argtypes = [c_void_p]
        self.dll.GrannyGetFileInfo.restype = c_void_p
        self.dll.GrannyFreeFile.argtypes = [c_void_p]
        self.dll.GrannyGetTotalObjectSize.argtypes = [c_void_p]
        self.dll.GrannyGetTotalObjectSize.restype = c_int32
        self.dll.GrannyGetMemberTypeSize.argtypes = [c_void_p]
        self.dll.GrannyGetMemberTypeSize.restype = c_int32

    def walk(self, data):
        """Read a file and describe what GrannyGetFileInfo gave back.

        None means the implementation refused the file. Every value here is
        either a name, a count, an index or a float32, so two walks compare
        exactly; nothing derived from an address appears, since those differ
        between implementations by construction.
        """
        buf = C.create_string_buffer(data, len(data))
        handle = self.dll.GrannyReadEntireFileFromMemory(len(data), C.cast(buf, c_void_p))
        if not handle:
            return None
        try:
            raw = self.dll.GrannyGetFileInfo(handle)
            if not raw:
                return {'FileInfo': None}
            return self._file_info(C.cast(raw, POINTER(FileInfo)).contents)
        finally:
            self.dll.GrannyFreeFile(handle)

    # The walk itself. Split up so a difference report names something readable.

    def _file_info(self, info):
        # Mesh identity is compared as an index into the file's own mesh array,
        # which is the only way to compare it at all: the engine checks
        # MeshBindings[i].Mesh against a pointer, and the pointers themselves are
        # different numbers in the two implementations.
        meshes = [C.addressof(info.Meshes[i].contents) for i in range(info.MeshCount)]

        return {
            'FromFileName': _s(info.FromFileName),
            'Counts': {
                'Textures': info.TextureCount, 'Materials': info.MaterialCount,
                'Skeletons': info.SkeletonCount, 'VertexDatas': info.VertexDataCount,
                'TriTopologies': info.TriTopologyCount, 'Meshes': info.MeshCount,
                'Models': info.ModelCount,
            },
            'Skeletons': [self._skeleton(info.Skeletons[i].contents)
                          for i in range(info.SkeletonCount)],
            'Meshes': [self._mesh(info.Meshes[i].contents, meshes)
                       for i in range(info.MeshCount)],
            'Models': [self._model(info.Models[i].contents, meshes)
                       for i in range(info.ModelCount)],
        }

    def _skeleton(self, sk):
        return {
            'Name': _s(sk.Name),
            'LODType': sk.LODType,
            'BoneCount': sk.BoneCount,
            'Bones': [self._bone(sk.Bones[i]) for i in range(sk.BoneCount)],
        }

    @staticmethod
    def _bone(bone):
        return {
            'Name': _s(bone.Name),
            'ParentIndex': bone.ParentIndex,
            'LODError': bone.LODError,
            'LocalTransform': _transform(bone.LocalTransform),
            'InverseWorld4x4': list(bone.InverseWorld4x4),
        }

    def _mesh(self, mesh, mesh_addresses):
        out = {
            'Name': _s(mesh.Name),
            'MorphTargetCount': mesh.MorphTargetCount,
            'MaterialBindingCount': mesh.MaterialBindingCount,
            'BoneBindingCount': mesh.BoneBindingCount,
            'BoneBindings': [],
        }
        for i in range(mesh.BoneBindingCount):
            b = mesh.BoneBindings[i]
            out['BoneBindings'].append({
                'BoneName': _s(b.BoneName),
                'OBBMin': list(b.OBBMin), 'OBBMax': list(b.OBBMax),
                'TriangleCount': b.TriangleCount,
                # The first few indices, which is enough to catch an array that
                # points somewhere else without copying megabytes per file.
                'FirstTriangleIndices': [b.TriangleIndices[k]
                                         for k in range(min(b.TriangleCount, 8))]
                                        if b.TriangleIndices else [],
            })

        v = mesh.PrimaryVertexData.contents if mesh.PrimaryVertexData else None
        if v is not None:
            stride = self.dll.GrannyGetTotalObjectSize(C.cast(v.VertexType, c_void_p))
            out['VertexData'] = {
                'VertexCount': v.VertexCount,
                'Stride': stride,
                'Components': self._components(v.VertexType),
                # The vertex bytes themselves, as words, at the stride the engine
                # would step with. Sampled rather than whole: a wrong stride or a
                # wrong pointer shows up in the first few and in the last.
                'Sample': _vertex_sample(v, stride),
            }

        t = mesh.PrimaryTopology.contents if mesh.PrimaryTopology else None
        if t is not None:
            out['Topology'] = {
                'GroupCount': t.GroupCount,
                'Groups': [[t.Groups[i].MaterialIndex, t.Groups[i].TriFirst,
                            t.Groups[i].TriCount] for i in range(t.GroupCount)],
                'IndexCount': t.IndexCount,
                'Index16Count': t.Index16Count,
                'FirstIndices': [t.Indices[i] for i in range(min(t.IndexCount, 12))]
                                if t.Indices else [],
                'LastIndices': [t.Indices[i] for i in
                                range(max(0, t.IndexCount - 12), t.IndexCount)]
                               if t.Indices else [],
            }
        return out

    def _components(self, vertex_type):
        out = []
        if not vertex_type:
            return out
        i = 0
        while vertex_type[i].Type != 0 and i < 64:
            p = vertex_type[i]
            out.append({
                'Name': _s(p.Name), 'Type': p.Type, 'ArrayWidth': p.ArrayWidth,
                'Size': self.dll.GrannyGetMemberTypeSize(
                    C.cast(C.byref(vertex_type[i]), c_void_p)),
            })
            i += 1
        return out

    def _model(self, model, mesh_addresses):
        bindings = []
        for i in range(model.MeshBindingCount):
            ptr = model.MeshBindings[i].Mesh
            if not ptr:
                bindings.append(None)
                continue
            address = C.addressof(ptr.contents)
            bindings.append({
                'Mesh': _s(ptr.contents.Name),
                # -1 would mean the model names a mesh the file does not list,
                # which is what breaks FindFirstAppropriateModel.
                'IndexInFileMeshes': mesh_addresses.index(address)
                                     if address in mesh_addresses else -1,
            })
        return {
            'Name': _s(model.Name),
            'Skeleton': _s(model.Skeleton.contents.Name) if model.Skeleton else None,
            'InitialPlacement': _transform(model.InitialPlacement),
            'MeshBindingCount': model.MeshBindingCount,
            'MeshBindings': bindings,
        }


def _s(value):
    return value.decode('utf-8', 'replace') if value else None


def _transform(t):
    return {'Flags': t.Flags, 'Position': list(t.Position),
            'Orientation': list(t.Orientation),
            'ScaleShear': [x for row in t.ScaleShear for x in row]}


def _vertex_sample(v, stride):
    """The first and last vertices, as raw words at the given stride."""
    if not v.Vertices or v.VertexCount <= 0 or stride < 4:
        return []
    words = stride // 4
    out = []
    for index in (0, v.VertexCount - 1):
        base = index * words
        out.append([v.Vertices[base + k] for k in range(min(words, 16))])
    return out


# --- comparing --------------------------------------------------------------

def _f32(value):
    """The float32 a number rounds to, so two printings of one value agree."""
    return struct.unpack('<f', struct.pack('<f', float(value)))[0]


def differences(a, b, path=''):
    """Every place the two disagree, as dotted paths."""
    out = []
    if isinstance(a, dict) and isinstance(b, dict):
        for key in sorted(set(a) | set(b)):
            if key not in a:
                out.append(('%s.%s' % (path, key), 'absent', 'present'))
            elif key not in b:
                out.append(('%s.%s' % (path, key), 'present', 'absent'))
            else:
                out += differences(a[key], b[key], '%s.%s' % (path, key))
    elif isinstance(a, list) and isinstance(b, list):
        if len(a) != len(b):
            out.append((path + '[]', '%d entries' % len(a), '%d entries' % len(b)))
        else:
            for i, (x, y) in enumerate(zip(a, b)):
                out += differences(x, y, '%s[%d]' % (path, i))
    elif isinstance(a, bool) or isinstance(b, bool):
        if a is not b:
            out.append((path, a, b))
    elif isinstance(a, (int, float)) and isinstance(b, (int, float)):
        # As float32, which is what both sides hold. Comparing the decimal forms
        # would report differences that are not there.
        if _f32(a) != _f32(b):
            out.append((path, a, b))
    elif a != b:
        out.append((path, a, b))
    return out


def generalise(path):
    """A difference path with its indices removed, for the histogram.

    Skeletons[0].Bones[97].LODError and Skeletons[0].Bones[3].LODError are one
    kind of problem, not two.
    """
    out = []
    depth = 0
    for ch in path:
        if ch == '[':
            depth += 1
            out.append('[')
        elif ch == ']':
            depth -= 1
            out.append(']')
        elif depth == 0:
            out.append(ch)
    return ''.join(out)


# --- the corpus -------------------------------------------------------------

def iter_corpus(roots, pattern):
    """Yield (label, bytes) for every GR2 under the given roots.

    A root may be a directory, which is searched for .pak archives and for loose
    GR2 files, or a .pak, or a single file.
    """
    for root in roots:
        if not os.path.exists(root):
            continue
        if os.path.isfile(root):
            if root.lower().endswith('.pak'):
                for item in _iter_pak(root, pattern):
                    yield item
            else:
                yield root, open(root, 'rb').read()
            continue

        for dirpath, _dirnames, filenames in os.walk(root):
            for name in sorted(filenames):
                full = os.path.join(dirpath, name)
                if name.lower().endswith('.pak'):
                    for item in _iter_pak(full, pattern):
                        yield item
                elif _looks_like_gr2_path(full):
                    try:
                        with open(full, 'rb') as f:
                            data = f.read()
                    except OSError:
                        continue
                    if data[:16] == MAGIC and fnmatch.fnmatch(full, pattern):
                        yield full, data


def _looks_like_gr2_path(path):
    normalised = path.replace('\\', '/')
    return any(d in normalised for d in GR2_DIRECTORIES)


def _iter_pak(path, pattern):
    try:
        archive = zipfile.ZipFile(path)
    except (zipfile.BadZipFile, OSError):
        return
    for name in archive.namelist():
        if not any(d in name for d in GR2_DIRECTORIES):
            continue
        label = '%s!%s' % (os.path.basename(path), name)
        if not fnmatch.fnmatch(label, pattern):
            continue
        try:
            data = archive.read(name)
        except (zipfile.BadZipFile, OSError, RuntimeError):
            continue
        if len(data) >= 16 and data[:16] == MAGIC:
            yield label, data


def codecs_used(data):
    """Which compression types a file's non-empty sections use."""
    if len(data) < 88:
        return set()
    array_offset, count = struct.unpack_from('<2I', data, 44)
    base = 32 + array_offset
    if base + 44 * count > len(data):
        return set()
    used = set()
    for i in range(count):
        fields = struct.unpack_from('<11I', data, base + 44 * i)
        if fields[3]:
            used.add(fields[0])
    return used


# --- the run ----------------------------------------------------------------

_WORKER = {}


def _init_worker(reference, candidate):
    _WORKER['reference'] = Granny(reference)
    _WORKER['candidate'] = Granny(candidate)


def _compare_one(item):
    label, data = item
    reference = _WORKER['reference']
    candidate = _WORKER['candidate']

    try:
        want = reference.walk(data)
    except Exception as e:                                  # noqa: BLE001
        return label, 'reference-crashed', ['%s' % e]
    try:
        got = candidate.walk(data)
    except Exception as e:                                  # noqa: BLE001
        return label, 'candidate-crashed', ['%s' % e]

    if want is None and got is None:
        return label, 'both-refused', []
    if want is None:
        return label, 'reference-refused', []
    if got is None:
        # Almost always one milestone away rather than wrong, so say which.
        unsupported = sorted(codecs_used(data) - {0, 2})
        if unsupported:
            names = {1: 'Oodle0', 3: 'BitKnit', 4: 'BitKnit2'}
            return label, 'unsupported/%s' % ','.join(
                names.get(c, str(c)) for c in unsupported), []
        return label, 'candidate-refused', []

    found = differences(want, got)
    return label, ('same' if not found else 'differs'), found


def main(argv):
    parser = argparse.ArgumentParser(
        description='Compare two granny2 implementations over a corpus of .gr2 files.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__.split('\n\n', 1)[1])
    parser.add_argument('roots', nargs='*', default=None,
                        help='directories, .pak archives or .gr2 files')
    parser.add_argument('--reference', default=DEFAULT_REFERENCE)
    parser.add_argument('--candidate', default=DEFAULT_CANDIDATE)
    parser.add_argument('--limit', type=int, default=0, help='stop after this many')
    parser.add_argument('--jobs', type=int, default=1,
                        help='worker processes; each loads both DLLs')
    parser.add_argument('--pattern', default='*', help='only labels matching this glob')
    parser.add_argument('--report', default=None, help='write every difference here')
    parser.add_argument('--show', type=int, default=10, help='differing files to print')
    args = parser.parse_args(argv[1:])

    if struct.calcsize('P') != 8:
        sys.stderr.write('gr2diff: needs a 64-bit Python for these DLLs\n')
        return 2
    for path in (args.reference, args.candidate):
        if not os.path.exists(path):
            sys.stderr.write('gr2diff: no such DLL: %s\n' % path)
            return 2

    roots = args.roots or DEFAULT_CORPUS
    print('reference %s' % args.reference)
    print('candidate %s' % args.candidate)
    print('corpus    %s\n' % ', '.join(roots))

    # Deduplicated by content: the three installs share most of their data, and
    # the mods duplicate more of it.
    def unique():
        seen = set()
        for label, data in iter_corpus(roots, args.pattern):
            digest = hashlib.sha256(data).digest()
            if digest in seen:
                continue
            seen.add(digest)
            yield label, data
            if args.limit and len(seen) >= args.limit:
                return

    outcomes = Counter()
    histogram = Counter()
    examples = {}
    differing = []

    if args.jobs > 1:
        pool = multiprocessing.Pool(args.jobs, _init_worker,
                                    (args.reference, args.candidate))
        results = pool.imap_unordered(_compare_one, unique(), chunksize=8)
    else:
        _init_worker(args.reference, args.candidate)
        results = (_compare_one(item) for item in unique())

    total = 0
    for label, outcome, found in results:
        total += 1
        outcomes[outcome] += 1
        if outcome == 'differs':
            differing.append((label, found))
            for path, want, got in found:
                key = generalise(path)
                histogram[key] += 1
                examples.setdefault(key, (label, want, got))
        if total % 2000 == 0:
            sys.stderr.write('  %d files...\n' % total)

    if args.jobs > 1:
        pool.close()
        pool.join()

    print('%d unique GR2 files\n' % total)
    for outcome, count in outcomes.most_common():
        print('  %-28s %6d  %5.1f%%' % (outcome, count, 100.0 * count / max(total, 1)))

    if histogram:
        print('\nwhat differs, by field:')
        for key, count in histogram.most_common(30):
            label, want, got = examples[key]
            print('  %-46s %5d files' % (key, count))
            print('      e.g. %s: reference %r, candidate %r' % (label, want, got))

    if args.show and differing:
        print('\nfirst %d differing files:' % min(args.show, len(differing)))
        for label, found in differing[:args.show]:
            print('  %s' % label)
            for path, want, got in found[:4]:
                print('      %s: %r vs %r' % (path, want, got))
            if len(found) > 4:
                print('      ... %d more' % (len(found) - 4))

    if args.report:
        with io.open(args.report, 'w', encoding='utf-8') as f:
            json.dump({'outcomes': dict(outcomes),
                       'histogram': dict(histogram),
                       'differences': [{'file': label,
                                        'differences': [{'path': p, 'reference': w,
                                                         'candidate': g}
                                                        for p, w, g in found]}
                                       for label, found in differing]},
                      f, indent=1, default=str)
        print('\nwrote %s' % args.report)

    return 1 if outcomes['differs'] or outcomes['candidate-crashed'] else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
