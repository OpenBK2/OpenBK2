"""Drive the real RAD granny2.dll from Python, as a reference oracle.

The GR2 replacement work needs something authoritative to compare against. The
shipped granny2.dll is exactly that, and it turns out to be scriptable: every
entry point the port needs is a plain C function, and `granny_int32x` is 32-bit
even on x64, so ctypes can call it with no thunking.

Two things bite when doing this by hand, and both are handled below.

  * **Granny structs are packed.** granny211.h asserts it indirectly, with
    `GrannyTypeSizeCheck(sizeof(T) == sizeof(a) + sizeof(b) + ...)` for every
    struct. Reading them at natural alignment does not fail loudly, it returns
    plausible garbage: the first attempt here read `SkeletonCount` as 398
    instead of 1. Every Structure below sets `_pack_ = 1`.
  * **Compressed sections need padding.** `GrannyGetCompressedBytesPaddingSize`
    returns 4 for both Oodle codecs; the decoder reads past the compressed
    length and will fault without it.

Usage:

    python granny_dll_oracle.py hash <file.gr2> ...
    python granny_dll_oracle.py pose <file.gr2> <t0> <t1> ...

`hash` prints the SHA-256 of every decompressed section, concatenated, which is
what a decoder under test has to reproduce byte for byte. `pose` dumps local
transforms, world matrices and composite skinning matrices as JSON, for
comparing a pose implementation with a tolerance.

Requires a 64-bit Python and the Windows DLL. See docs/GrannyReplacement.md.
"""

import ctypes as C
import hashlib
import json
import os
import struct
import sys
from ctypes import POINTER, c_bool, c_char_p, c_float, c_int32, c_uint32, c_void_p

DLL_PATH = os.environ.get(
    "GRANNY2_DLL",
    os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..",
                 "third_party", "uesp-esoapps", "common", "granny",
                 "win64", "granny2_x64.dll"),
)

g = C.CDLL(os.path.abspath(DLL_PATH))


# --- structures ------------------------------------------------------------
#
# Layouts from granny211.h. All packed, see module docstring.

class Variant(C.Structure):
    _pack_ = 1
    _fields_ = [("Type", c_void_p), ("Object", c_void_p)]


class Transform(C.Structure):
    _pack_ = 1
    _fields_ = [("Flags", c_uint32), ("Position", c_float * 3),
                ("Orientation", c_float * 4), ("ScaleShear", (c_float * 3) * 3)]


class Bone(C.Structure):
    _pack_ = 1
    _fields_ = [("Name", c_char_p), ("ParentIndex", c_int32),
                ("LocalTransform", Transform), ("InverseWorld4x4", c_float * 16),
                ("LODError", c_float), ("ExtendedData", Variant)]


class Skeleton(C.Structure):
    _pack_ = 1
    _fields_ = [("Name", c_char_p), ("BoneCount", c_int32), ("Bones", POINTER(Bone)),
                ("LODType", c_int32), ("ExtendedData", Variant)]


class Model(C.Structure):
    _pack_ = 1
    _fields_ = [("Name", c_char_p), ("Skeleton", POINTER(Skeleton)),
                ("InitialPlacement", Transform), ("MeshBindingCount", c_int32),
                ("MeshBindings", c_void_p), ("ExtendedData", Variant)]


class Animation(C.Structure):
    _pack_ = 1
    _fields_ = [("Name", c_char_p), ("Duration", c_float), ("TimeStep", c_float),
                ("Oversampling", c_float), ("TrackGroupCount", c_int32),
                ("TrackGroups", c_void_p), ("DefaultLoopCount", c_int32),
                ("Flags", c_int32), ("ExtendedData", Variant)]


class FileInfo(C.Structure):
    _pack_ = 1
    _fields_ = [
        ("ArtToolInfo", c_void_p), ("ExporterInfo", c_void_p), ("FromFileName", c_char_p),
        ("TextureCount", c_int32), ("Textures", c_void_p),
        ("MaterialCount", c_int32), ("Materials", c_void_p),
        ("SkeletonCount", c_int32), ("Skeletons", POINTER(POINTER(Skeleton))),
        ("VertexDataCount", c_int32), ("VertexDatas", c_void_p),
        ("TriTopologyCount", c_int32), ("TriTopologies", c_void_p),
        ("MeshCount", c_int32), ("Meshes", c_void_p),
        ("ModelCount", c_int32), ("Models", POINTER(POINTER(Model))),
        ("TrackGroupCount", c_int32), ("TrackGroups", c_void_p),
        ("AnimationCount", c_int32), ("Animations", POINTER(POINTER(Animation))),
        ("ExtendedData", Variant),
    ]


# Sanity: these must match the arithmetic granny211.h asserts.
_EXPECTED_SIZES = {Variant: 16, Transform: 68, Bone: 164, Skeleton: 40,
                   Model: 112, Animation: 56, FileInfo: 148}
for _t, _n in _EXPECTED_SIZES.items():
    assert C.sizeof(_t) == _n, f"{_t.__name__} is {C.sizeof(_t)}, expected {_n}"


# --- entry points ----------------------------------------------------------

_SIGNATURES = {
    "GrannyGetCompressedBytesPaddingSize": ([c_int32], c_int32),
    "GrannyDecompressData": ([c_int32, c_bool, c_int32, c_void_p,
                              c_int32, c_int32, c_int32, c_void_p], c_bool),
    "GrannyReadEntireFileFromMemory": ([c_int32, c_void_p], c_void_p),
    "GrannyGetFileInfo": ([c_void_p], POINTER(FileInfo)),
    "GrannyFreeFile": ([c_void_p], None),
    "GrannyInstantiateModel": ([POINTER(Model)], c_void_p),
    "GrannyFreeModelInstance": ([c_void_p], None),
    "GrannySetModelClock": ([c_void_p, c_float], None),
    "GrannyBeginControlledAnimation": ([c_float, POINTER(Animation)], c_void_p),
    "GrannySetTrackGroupTarget": ([c_void_p, c_int32, c_void_p], None),
    "GrannyEndControlledAnimation": ([c_void_p], c_void_p),
    "GrannyNewLocalPose": ([c_int32], c_void_p),
    "GrannyFreeLocalPose": ([c_void_p], None),
    "GrannyGetLocalPoseTransform": ([c_void_p, c_int32], POINTER(Transform)),
    "GrannySampleModelAnimations": ([c_void_p, c_int32, c_int32, c_void_p], None),
    "GrannyNewWorldPose": ([c_int32], c_void_p),
    "GrannyFreeWorldPose": ([c_void_p], None),
    "GrannyBuildWorldPose": ([POINTER(Skeleton), c_int32, c_int32, c_void_p,
                             POINTER(c_float), c_void_p], None),
    "GrannyGetWorldPose4x4": ([c_void_p, c_int32], POINTER(c_float)),
    "GrannyGetWorldPoseComposite4x4": ([c_void_p, c_int32], POINTER(c_float)),
}
for _name, (_argtypes, _restype) in _SIGNATURES.items():
    _fn = getattr(g, _name)
    _fn.argtypes = _argtypes
    _fn.restype = _restype

IDENTITY_4X4 = (c_float * 16)(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1)


# --- container -------------------------------------------------------------

def sections(data):
    """Yield (index, compression, data_offset, data_size, expanded_size,
    alignment, stop0, stop1) for each section in a GR2 blob."""
    file_info_size = struct.unpack_from("<I", data, 44)[0]
    count = struct.unpack_from("<I", data, 48)[0]
    for i in range(count):
        yield (i,) + struct.unpack_from("<7I", data, 32 + file_info_size + i * 44)


def decompress_section(data, section):
    """Decompress one section through the DLL. Dispatches per section, not per
    file: mixed-compression files exist in the retail data."""
    _, comp, offset, clen, dlen, _align, stop0, stop1 = section
    if dlen == 0:
        return b""
    if comp == 0:
        return data[offset:offset + dlen]
    pad = g.GrannyGetCompressedBytesPaddingSize(comp)
    src = C.create_string_buffer(data[offset:offset + clen] + b"\0" * pad, clen + pad)
    dst = C.create_string_buffer(dlen)
    ok = g.GrannyDecompressData(comp, False, clen, C.cast(src, c_void_p),
                                stop0, stop1, dlen, C.cast(dst, c_void_p))
    if not ok:
        raise RuntimeError(f"GrannyDecompressData failed (section {section[0]}, format {comp})")
    return dst.raw[:dlen]


def file_hash(path):
    """SHA-256 over every decompressed section, concatenated in order."""
    data = open(path, "rb").read()
    h = hashlib.sha256()
    for s in sections(data):
        if s[4] == 0:
            continue
        h.update(decompress_section(data, s))
    return h.hexdigest()


# --- pose ------------------------------------------------------------------

def pose(path, times):
    """Sample the first animation of a file at the given times.

    Returns local transforms, world matrices and composite skinning matrices
    per bone per time, i.e. the output a replacement's pose layer must match.
    """
    raw = open(path, "rb").read()
    buf = C.create_string_buffer(raw, len(raw))
    handle = g.GrannyReadEntireFileFromMemory(len(raw), C.cast(buf, c_void_p))
    if not handle:
        raise RuntimeError("GrannyReadEntireFileFromMemory failed")
    info = g.GrannyGetFileInfo(handle).contents
    if info.ModelCount == 0:
        raise RuntimeError("file has no models")
    model = info.Models[0]
    skeleton = model.contents.Skeleton
    nbones = skeleton.contents.BoneCount
    animation = info.Animations[0] if info.AnimationCount else None

    instance = g.GrannyInstantiateModel(model)
    if animation:
        builder = g.GrannyBeginControlledAnimation(0.0, animation)
        for tg in range(animation.contents.TrackGroupCount):
            g.GrannySetTrackGroupTarget(builder, tg, instance)
        g.GrannyEndControlledAnimation(builder)

    local_pose = g.GrannyNewLocalPose(nbones)
    world_pose = g.GrannyNewWorldPose(nbones)
    out = {
        "file": path,
        "skeleton": skeleton.contents.Name.decode("latin1"),
        "bones": [skeleton.contents.Bones[i].Name.decode("latin1") for i in range(nbones)],
        "anim": animation.contents.Name.decode("latin1") if animation else None,
        "duration": animation.contents.Duration if animation else 0.0,
        "frames": [],
    }
    for t in times:
        g.GrannySetModelClock(instance, c_float(t))
        g.GrannySampleModelAnimations(instance, 0, nbones, local_pose)
        g.GrannyBuildWorldPose(skeleton, 0, nbones, local_pose, IDENTITY_4X4, world_pose)
        frame = {"t": t, "local": [], "world": [], "skin": []}
        for i in range(nbones):
            tr = g.GrannyGetLocalPoseTransform(local_pose, i).contents
            frame["local"].append({
                "flags": tr.Flags,
                "pos": list(tr.Position),
                "quat": list(tr.Orientation),          # x, y, z, w
                "ss": [list(row) for row in tr.ScaleShear],
            })
            frame["world"].append([g.GrannyGetWorldPose4x4(world_pose, i)[k] for k in range(16)])
            frame["skin"].append([g.GrannyGetWorldPoseComposite4x4(world_pose, i)[k] for k in range(16)])
        out["frames"].append(frame)

    g.GrannyFreeWorldPose(world_pose)
    g.GrannyFreeLocalPose(local_pose)
    g.GrannyFreeModelInstance(instance)
    g.GrannyFreeFile(handle)
    return out


def main(argv):
    if len(argv) < 3 or argv[1] not in ("hash", "pose"):
        print(__doc__)
        return 1
    if argv[1] == "hash":
        for path in argv[2:]:
            print(f"{path} {file_hash(path)}")
        return 0
    path = argv[2]
    times = [float(x) for x in argv[3:]] or [0.0]
    print(json.dumps(pose(path, times)))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
