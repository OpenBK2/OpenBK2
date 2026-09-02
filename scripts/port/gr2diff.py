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

Output is a histogram before it is a list: over twenty thousand files, what is
useful is "InverseWorld4x4 differs in 12 files", not twelve thousand lines.

Files whose only differences are float rounding are reported as "near" with the
worst relative difference, rather than as failures. Transform composition is
float arithmetic and two correct implementations differ in the last bit or two;
the real DLL is not reliably the more accurate one. --tolerance sets where that
line falls. Everything derived by copying bytes still compares exactly, because
there rounding is not a thing that can happen.
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
from ctypes import (POINTER, c_bool, c_char_p, c_float, c_int16, c_int32, c_uint8,
                    c_uint16, c_uint32, c_void_p)

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


class Curve2(C.Structure):
    _pack_ = 1
    _fields_ = [('CurveData', Variant)]


class CurveDataDaK32fC32f(C.Structure):
    """The one curve format these files produce: floats in, floats out.

    Granny has eighteen, most of them quantised. A 2.5 curve is a
    granny_old_curve, a degree and two float arrays, and this is the 2.11 format
    with the same three fields, which is what the real DLL turns every one of
    them into.
    """
    _pack_ = 1
    _fields_ = [('Format', c_uint8), ('Degree', c_uint8), ('Padding', c_int16),
                ('KnotCount', c_int32), ('Knots', POINTER(c_float)),
                ('ControlCount', c_int32), ('Controls', POINTER(c_float))]


class VectorTrack(C.Structure):
    _pack_ = 1
    _fields_ = [('Name', c_char_p), ('TrackKey', c_uint32), ('Dimension', c_int32),
                ('ValueCurve', Curve2)]


class TransformTrack(C.Structure):
    _pack_ = 1
    _fields_ = [('Name', c_char_p), ('Flags', c_int32), ('OrientationCurve', Curve2),
                ('PositionCurve', Curve2), ('ScaleShearCurve', Curve2)]


class TextTrackEntry(C.Structure):
    _pack_ = 1
    _fields_ = [('TimeStamp', c_float), ('Text', c_char_p)]


class TextTrack(C.Structure):
    _pack_ = 1
    _fields_ = [('Name', c_char_p), ('EntryCount', c_int32),
                ('Entries', POINTER(TextTrackEntry))]


class PeriodicLoop(C.Structure):
    _pack_ = 1
    _fields_ = [('Radius', c_float), ('dAngle', c_float), ('dZ', c_float),
                ('BasisX', c_float * 3), ('BasisY', c_float * 3), ('Axis', c_float * 3)]


class TrackGroup(C.Structure):
    _pack_ = 1
    _fields_ = [
        ('Name', c_char_p),
        ('VectorTrackCount', c_int32), ('VectorTracks', POINTER(VectorTrack)),
        ('TransformTrackCount', c_int32), ('TransformTracks', POINTER(TransformTrack)),
        ('TransformLODErrorCount', c_int32), ('TransformLODErrors', POINTER(c_float)),
        ('TextTrackCount', c_int32), ('TextTracks', POINTER(TextTrack)),
        ('InitialPlacement', Transform),
        ('Flags', c_int32), ('LoopTranslation', c_float * 3),
        ('PeriodicLoop', POINTER(PeriodicLoop)), ('ExtendedData', Variant),
    ]


class Animation(C.Structure):
    _pack_ = 1
    _fields_ = [
        ('Name', c_char_p), ('Duration', c_float), ('TimeStep', c_float),
        ('Oversampling', c_float),
        ('TrackGroupCount', c_int32), ('TrackGroups', POINTER(POINTER(TrackGroup))),
        ('DefaultLoopCount', c_int32), ('Flags', c_int32), ('ExtendedData', Variant),
    ]


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
        ('TrackGroupCount', c_int32), ('TrackGroups', POINTER(POINTER(TrackGroup))),
        ('AnimationCount', c_int32), ('Animations', POINTER(POINTER(Animation))),
        ('ExtendedData', Variant),
    ]


_SIZES = ((Variant, 16), (Transform, 68), (DataType, 44), (Bone, 164), (Skeleton, 40),
          (Model, 112), (FileInfo, 148), (Mesh, 76), (VertexData, 44),
          (TriTopology, 132), (BoneBinding, 44), (TriMaterialGroup, 12),
          (Curve2, 16), (CurveDataDaK32fC32f, 28), (VectorTrack, 32),
          (TransformTrack, 60), (TextTrackEntry, 12), (TextTrack, 20),
          (PeriodicLoop, 48), (TrackGroup, 164), (Animation, 56))
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
        # The small queries. Compared as well as the structures, because the
        # engine calls them and a structure that reads correctly says nothing
        # about a function that summarises it.
        self.dll.GrannyGetMeshTriangleGroupCount.argtypes = [c_void_p]
        self.dll.GrannyGetMeshTriangleGroupCount.restype = c_int32
        self.dll.GrannyMeshIsRigid.argtypes = [c_void_p]
        self.dll.GrannyMeshIsRigid.restype = c_bool
        for name, restype in [('GrannyGetMeshVertexCount', c_int32),
                              ('GrannyGetMeshVertices', c_void_p),
                              ('GrannyGetMeshVertexType', c_void_p),
                              ('GrannyGetMeshIndexCount', c_int32),
                              ('GrannyGetMeshIndices', c_void_p),
                              ('GrannyGetMeshBytesPerIndex', c_int32)]:
            fn = getattr(self.dll, name)
            fn.argtypes = [c_void_p]
            fn.restype = restype
        self.dll.GrannyFindBoneByName.argtypes = [c_void_p, c_char_p, POINTER(c_int32)]
        self.dll.GrannyFindBoneByName.restype = c_bool
        self.dll.GrannyMakeIdentity.argtypes = [c_void_p]
        self.dll.GrannyPostMultiplyBy.argtypes = [c_void_p, c_void_p]
        # The pose path, as far as it goes. Sampling with nothing bound produces
        # the skeleton's rest pose, which is what the game gets today and what
        # every model will be drawn in until controls exist.
        self.dll.GrannyInstantiateModel.argtypes = [c_void_p]
        self.dll.GrannyInstantiateModel.restype = c_void_p
        self.dll.GrannyFreeModelInstance.argtypes = [c_void_p]
        self.dll.GrannySetModelClock.argtypes = [c_void_p, c_float]
        self.dll.GrannyNewLocalPose.argtypes = [c_int32]
        self.dll.GrannyNewLocalPose.restype = c_void_p
        self.dll.GrannyFreeLocalPose.argtypes = [c_void_p]
        self.dll.GrannyGetLocalPoseBoneCount.argtypes = [c_void_p]
        self.dll.GrannyGetLocalPoseBoneCount.restype = c_int32
        self.dll.GrannyGetLocalPoseTransform.argtypes = [c_void_p, c_int32]
        self.dll.GrannyGetLocalPoseTransform.restype = c_void_p
        self.dll.GrannySampleModelAnimations.argtypes = [c_void_p, c_int32, c_int32,
                                                         c_void_p]
        self.dll.GrannyNewWorldPose.argtypes = [c_int32]
        self.dll.GrannyNewWorldPose.restype = c_void_p
        self.dll.GrannyFreeWorldPose.argtypes = [c_void_p]
        self.dll.GrannyBuildWorldPose.argtypes = [c_void_p, c_int32, c_int32, c_void_p,
                                                  c_void_p, c_void_p]
        self.dll.GrannyGetWorldPose4x4.argtypes = [c_void_p, c_int32]
        self.dll.GrannyGetWorldPose4x4.restype = c_void_p
        self.dll.GrannyGetWorldPoseComposite4x4.argtypes = [c_void_p, c_int32]
        self.dll.GrannyGetWorldPoseComposite4x4.restype = c_void_p
        self.dll.GrannyEvaluateCurveAtT.argtypes = [c_int32, c_bool, c_bool, c_void_p,
                                                    c_bool, c_float, c_float,
                                                    POINTER(c_float), POINTER(c_float)]
        self.dll.GrannyEvaluateCurveAtT.restype = None

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
        # The same trick for track groups. An animation's TrackGroups point at
        # the file's own, in all 11,360 groups of the corpus, and an
        # implementation that converted each one twice would still read
        # correctly while breaking anything that compares the pointers.
        groups = [C.addressof(info.TrackGroups[i].contents)
                  for i in range(info.TrackGroupCount)]

        return {
            'FromFileName': _s(info.FromFileName),
            'Counts': {
                'Textures': info.TextureCount, 'Materials': info.MaterialCount,
                'Skeletons': info.SkeletonCount, 'VertexDatas': info.VertexDataCount,
                'TriTopologies': info.TriTopologyCount, 'Meshes': info.MeshCount,
                'Models': info.ModelCount, 'TrackGroups': info.TrackGroupCount,
                'Animations': info.AnimationCount,
            },
            # The duration a curve loops over is the animation's, so it comes
            # from the first animation that reaches this group rather than from
            # the group itself, which does not carry one.
            'TrackGroups': [self._track_group(info.TrackGroups[i],
                                              _group_duration(info, i))
                            for i in range(info.TrackGroupCount)],
            'Animations': [self._animation(info.Animations[i].contents, groups)
                           for i in range(info.AnimationCount)],
            'Skeletons': [self._skeleton(info.Skeletons[i])
                          for i in range(info.SkeletonCount)],
            'Meshes': [self._mesh(info.Meshes[i], meshes)
                       for i in range(info.MeshCount)],
            'Models': [self._model(info.Models[i].contents, meshes)
                       for i in range(info.ModelCount)],
            'Poses': [self.sampled_pose(info.Models[i])
                      for i in range(info.ModelCount)],
        }

    def _skeleton(self, skeleton_ptr):
        sk = skeleton_ptr.contents
        out = {
            'Name': _s(sk.Name),
            'LODType': sk.LODType,
            'BoneCount': sk.BoneCount,
            'Bones': [self._bone(sk.Bones[i]) for i in range(sk.BoneCount)],
        }

        # GrannyFindBoneByName, for every bone by its own name and for one that
        # is not there. The engine turns a mesh's bone binding name into an index
        # this way, so a wrong answer is a limb on the wrong joint.
        address = C.cast(skeleton_ptr, c_void_p)
        lookups = []
        for i in range(min(sk.BoneCount, 64)):
            index = c_int32(-99)
            found = self.dll.GrannyFindBoneByName(address, sk.Bones[i].Name,
                                                  C.byref(index))
            lookups.append([bool(found), index.value])
        index = c_int32(-99)
        found = self.dll.GrannyFindBoneByName(address, b'\x01no such bone',
                                              C.byref(index))
        lookups.append([bool(found), index.value])
        out['FindBoneByName'] = lookups
        return out

    @staticmethod
    def _bone(bone):
        return {
            'Name': _s(bone.Name),
            'ParentIndex': bone.ParentIndex,
            'LODError': bone.LODError,
            'LocalTransform': _transform(bone.LocalTransform),
            'InverseWorld4x4': list(bone.InverseWorld4x4),
        }

    def _animation(self, animation, group_addresses):
        return {
            'Name': _s(animation.Name),
            'Duration': animation.Duration,
            'TimeStep': animation.TimeStep,
            'Oversampling': animation.Oversampling,
            'DefaultLoopCount': animation.DefaultLoopCount,
            'Flags': animation.Flags,
            'HasExtendedData': bool(animation.ExtendedData.Type
                                    or animation.ExtendedData.Object),
            'TrackGroupCount': animation.TrackGroupCount,
            # As indices into the file's own array, which is the only comparable
            # form: the addresses themselves differ between implementations.
            'TrackGroupIndices': [
                group_addresses.index(C.addressof(animation.TrackGroups[i].contents))
                if C.addressof(animation.TrackGroups[i].contents) in group_addresses
                else -1
                for i in range(animation.TrackGroupCount)],
        }

    def _track_group(self, group_ptr, duration=0.0):
        g = group_ptr.contents
        out = {
            'Name': _s(g.Name),
            'Flags': g.Flags,
            'InitialPlacement': _transform(g.InitialPlacement),
            'LoopTranslation': list(g.LoopTranslation),
            'TransformLODErrorCount': g.TransformLODErrorCount,
            'TransformLODErrors': [g.TransformLODErrors[i]
                                   for i in range(min(g.TransformLODErrorCount, 32))]
                                  if g.TransformLODErrors else [],
            'HasExtendedData': bool(g.ExtendedData.Type or g.ExtendedData.Object),
            'VectorTrackCount': g.VectorTrackCount,
            'TransformTrackCount': g.TransformTrackCount,
            'TextTrackCount': g.TextTrackCount,
        }
        if g.PeriodicLoop:
            p = g.PeriodicLoop.contents
            out['PeriodicLoop'] = {
                'Radius': p.Radius, 'dAngle': p.dAngle, 'dZ': p.dZ,
                'BasisX': list(p.BasisX), 'BasisY': list(p.BasisY),
                'Axis': list(p.Axis),
            }
        else:
            out['PeriodicLoop'] = None

        out['VectorTracks'] = [{
            'Name': _s(g.VectorTracks[i].Name),
            'TrackKey': g.VectorTracks[i].TrackKey,
            'Dimension': g.VectorTracks[i].Dimension,
            'ValueCurve': self._curve(g.VectorTracks[i].ValueCurve),
            'Sampled': self._sample_curve(g.VectorTracks[i].ValueCurve,
                                          max(g.VectorTracks[i].Dimension, 1),
                                          duration),
        } for i in range(g.VectorTrackCount)]

        # A transform track's three slots have fixed dimensions: a position is
        # always 3, an orientation always 4, a scale-shear always 9, and the
        # corpus has no exception in 772,767 curves.
        out['TransformTracks'] = [{
            'Name': _s(g.TransformTracks[i].Name),
            'Flags': g.TransformTracks[i].Flags,
            'PositionCurve': self._curve(g.TransformTracks[i].PositionCurve),
            'OrientationCurve': self._curve(g.TransformTracks[i].OrientationCurve),
            'ScaleShearCurve': self._curve(g.TransformTracks[i].ScaleShearCurve),
            'PositionSampled': self._sample_curve(g.TransformTracks[i].PositionCurve,
                                                  3, duration),
            'OrientationSampled': self._sample_curve(
                g.TransformTracks[i].OrientationCurve, 4, duration),
            'ScaleShearSampled': self._sample_curve(
                g.TransformTracks[i].ScaleShearCurve, 9, duration),
        } for i in range(g.TransformTrackCount)]

        out['TextTracks'] = [{
            'Name': _s(g.TextTracks[i].Name),
            'EntryCount': g.TextTracks[i].EntryCount,
            'Entries': [[g.TextTracks[i].Entries[e].TimeStamp,
                         _s(g.TextTracks[i].Entries[e].Text)]
                        for e in range(min(g.TextTracks[i].EntryCount, 64))]
                       if g.TextTracks[i].Entries else [],
        } for i in range(g.TextTrackCount)]
        return out

    def _sample_curve(self, curve, dim, duration):
        """GrannyEvaluateCurveAtT at values of t drawn from the curve's own knots.

        A wrong spline basis is invisible at a knot of an evenly spaced curve and
        obvious a quarter of the way into a span whose neighbour is a different
        length, so the samples are placed relative to each curve rather than on a
        fixed grid. The ends are where the knot sequence is clamped or wrapped,
        which is the part with no prior art, so both are sampled with the loop
        flags as well as without.

        t below the first knot is deliberately not sampled. Every one of the
        450,288 non-empty curves in the corpus starts at knot 0.0 and the engine's
        clocks are clamped to be non-negative, so it cannot arise; the real DLL
        answers from an essentially arbitrary span there and reproducing that
        would be reproducing a bug nothing can reach.
        """
        obj = curve.CurveData.Object
        if not obj:
            return None
        k = C.cast(obj, POINTER(CurveDataDaK32fC32f)).contents
        n = k.KnotCount
        if n <= 0 or not k.Knots:
            # An empty curve returns the identity vector it is given.
            identity = (c_float * dim)(*[float(i) + 1.5 for i in range(dim)])
            return {'Empty': self._evaluate(curve, dim, 0.0, identity=identity)}

        knots = [k.Knots[i] for i in range(n)]
        mid = n // 2
        ts = [knots[0]]
        if n > 1:
            span = knots[1] - knots[0]
            ts += [knots[0] + 0.25 * span, knots[0] + 0.5 * span, knots[1]]
            middle = knots[mid] - knots[mid - 1]
            ts += [knots[mid - 1] + 0.25 * middle, knots[mid - 1] + 0.75 * middle]
            last = knots[n - 1] - knots[n - 2]
            ts += [knots[n - 2] + 0.5 * last, knots[n - 1],
                   knots[n - 1] + 0.25 * last]

        out = {'Plain': [self._evaluate(curve, dim, t) for t in ts]}
        if n > 1:
            # The wrap, at both ends, which is where index n-1 and index 0 are
            # the same keyframe.
            ends = [knots[0], knots[0] + 0.5 * (knots[1] - knots[0]),
                    knots[n - 2], knots[n - 1]]
            out['Forwards'] = [self._evaluate(curve, dim, t, duration=duration,
                                              forwards=True) for t in ends]
            out['Backwards'] = [self._evaluate(curve, dim, t, duration=duration,
                                               backwards=True) for t in ends]
        if dim != 3:
            # Normalize is meaningful for a quaternion and is what the sampler
            # uses it for. At Dimension 3 the real DLL returns something that is
            # not a normalization at all: the divisor it implies ranges from
            # -10.6 to 47,000 times the vector's own length across the corpus and
            # sometimes flips its sign, so it is not a rule to reproduce. The
            # engine never makes that call, and neither does anything here.
            out['Normalized'] = self._evaluate(curve, dim, ts[len(ts) // 2],
                                               normalize=True)
        return out

    def _evaluate(self, curve, dim, t, duration=0.0, normalize=False,
                  forwards=False, backwards=False, identity=None):
        result = (c_float * dim)()
        self.dll.GrannyEvaluateCurveAtT(dim, normalize, backwards, C.byref(curve),
                                        forwards, duration, t, result,
                                        identity if identity is not None else None)
        return list(result)

    def _curve(self, curve):
        """A curve2, read through the type its variant points at.

        The type is the DLL's own global and its address means nothing across
        implementations, so what is compared is its member list: a variant with a
        null type is not the same object as one with a type, and the format byte
        is what tells eighteen curve encodings apart.
        """
        if not curve.CurveData.Type and not curve.CurveData.Object:
            return None
        out = {'TypeMembers': self._type_members(curve.CurveData.Type)}
        if not curve.CurveData.Object:
            out['Data'] = None
            return out
        k = C.cast(curve.CurveData.Object, POINTER(CurveDataDaK32fC32f)).contents
        out['Data'] = {
            'Format': k.Format, 'Degree': k.Degree,
            'KnotCount': k.KnotCount, 'ControlCount': k.ControlCount,
            # The whole of both arrays. They are the animation, and a curve that
            # points at the wrong offset in the file is the failure this is here
            # to catch; the largest in the corpus is a few hundred floats.
            'Knots': [k.Knots[i] for i in range(k.KnotCount)] if k.Knots else [],
            'Controls': [k.Controls[i] for i in range(k.ControlCount)]
                        if k.Controls else [],
        }
        # Padding is uninitialised in the real DLL, which returned 16414, -17102
        # and 0 for three curves of one file, so it is deliberately not compared.
        return out

    def _type_members(self, type_ptr, depth=0):
        # Depth capped because a type tree may name itself, and this walks
        # reference types rather than tracking which it has already seen.
        if not type_ptr or depth > 4:
            return None
        out = []
        p = C.cast(type_ptr, POINTER(DataType))
        i = 0
        while i < 64 and p[i].Type != 0:
            out.append([p[i].Type, _s(p[i].Name), p[i].ArrayWidth,
                        self._type_members(C.cast(p[i].ReferenceType, c_void_p).value,
                                           depth + 1)])
            i += 1
        return out

    def _mesh(self, mesh_ptr, mesh_addresses):
        mesh = mesh_ptr.contents
        address = C.cast(mesh_ptr, c_void_p)
        out = {
            'Name': _s(mesh.Name),
            'TriangleGroupCount': self.dll.GrannyGetMeshTriangleGroupCount(address),
            'IsRigid': bool(self.dll.GrannyMeshIsRigid(address)),
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

        # The six accessors, reported as what they say *relative to* the
        # structure, not as raw pointers: an address differs between two DLLs
        # for every mesh and would drown the comparison. "vertices" and
        # "indices" name which field the returned pointer equalled.
        out['Accessors'] = self._accessors(mesh, address)

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

    def _accessors(self, mesh, address):
        """What the six mesh accessors return, named against the structure."""
        v = mesh.PrimaryVertexData.contents if mesh.PrimaryVertexData else None
        t = mesh.PrimaryTopology.contents if mesh.PrimaryTopology else None

        def where(got, candidates):
            if not got:
                return 'null'
            for name, ptr in candidates:
                if ptr and C.cast(ptr, c_void_p).value == got:
                    return name
            return 'unknown'

        return {
            'VertexCount': self.dll.GrannyGetMeshVertexCount(address),
            'Vertices': where(self.dll.GrannyGetMeshVertices(address),
                              [('Vertices', v.Vertices if v else None)]),
            'VertexType': where(self.dll.GrannyGetMeshVertexType(address),
                                [('VertexType', v.VertexType if v else None)]),
            'IndexCount': self.dll.GrannyGetMeshIndexCount(address),
            'Indices': where(self.dll.GrannyGetMeshIndices(address),
                             [('Indices', t.Indices if t else None),
                              ('Indices16', t.Indices16 if t else None)]),
            'BytesPerIndex': self.dll.GrannyGetMeshBytesPerIndex(address),
        }

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

    def transforms(self):
        """GrannyMakeIdentity and GrannyPostMultiplyBy on authored inputs.

        Independent of any file, but checked through the same path so that
        pointing gr2diff at a new implementation covers them without a second
        tool. The inputs are the ones that told the two conventions apart:
        a rotation composed with a translation, a shear with a translation, and
        a scaled rotation with a scale.
        """
        s = 0.7071067811865476

        def make(flags, pos=(0, 0, 0), ori=(0, 0, 0, 1),
                 ss=(1, 0, 0, 0, 1, 0, 0, 0, 1)):
            return struct.pack('<I16f', flags, *(list(pos) + list(ori) + list(ss)))

        identity = C.create_string_buffer(68)
        self.dll.GrannyMakeIdentity(C.cast(identity, c_void_p))
        out = {'MakeIdentity': list(struct.unpack('<I16f', identity.raw[:68]))}

        cases = [
            (make(1, pos=(1, 2, 3)), make(1, pos=(10, 20, 30))),
            (make(2, ori=(0, 0, s, s)), make(1, pos=(1, 0, 0))),
            (make(2, ori=(0, 0, s, s)), make(2, ori=(s, 0, 0, s))),
            (make(4, ss=(1, 1, 0, 0, 1, 0, 0, 0, 1)), make(1, pos=(1, 0, 0))),
            (make(6, ori=(0, 0, s, s), ss=(2, 0, 0, 0, 3, 0, 0, 0, 1)),
             make(4, ss=(5, 0, 0, 0, 7, 0, 0, 0, 1))),
            (make(7, pos=(1, 2, 3), ori=(0, 0, s, s), ss=(2, 0, 0, 0, 2, 0, 0, 0, 2)),
             make(7, pos=(4, 5, 6), ori=(s, 0, 0, s), ss=(3, 0, 0, 0, 3, 0, 0, 0, 3))),
        ]
        out['PostMultiplyBy'] = []
        for a, b in cases:
            first = C.create_string_buffer(a, 68)
            second = C.create_string_buffer(b, 68)
            self.dll.GrannyPostMultiplyBy(C.cast(first, c_void_p),
                                          C.cast(second, c_void_p))
            out['PostMultiplyBy'].append(list(struct.unpack('<I16f', first.raw[:68])))
        return out

    def sampled_pose(self, model_ptr):
        """Instantiate a model, sample it with nothing bound, read the pose back.

        Every entry point on the render path that has one, in the order the engine
        calls them: InstantiateModel, NewLocalPose, SetModelClock,
        SampleModelAnimations, GetLocalPoseTransform. With no controls the answer
        is the skeleton's rest pose, so this checks the sampler's range handling
        and its copy against real skeletons rather than hand built ones.
        """
        model = model_ptr.contents
        if not model.Skeleton:
            return None
        bone_count = model.Skeleton.contents.BoneCount
        if bone_count <= 0:
            return {'BoneCount': bone_count}

        instance = self.dll.GrannyInstantiateModel(C.cast(model_ptr, c_void_p))
        if not instance:
            return {'Instantiated': False}
        pose = self.dll.GrannyNewLocalPose(bone_count)
        if not pose:
            self.dll.GrannyFreeModelInstance(instance)
            return {'Pose': False}

        out = {'BoneCount': self.dll.GrannyGetLocalPoseBoneCount(pose)}
        # A clock that is not zero, to confirm it changes nothing while nothing
        # is bound.
        self.dll.GrannySetModelClock(instance, 1.25)
        self.dll.GrannySampleModelAnimations(instance, 0, bone_count, pose)

        transforms = []
        for i in range(min(bone_count, 64)):
            p = self.dll.GrannyGetLocalPoseTransform(pose, i)
            transforms.append(list(struct.unpack('<I16f', C.string_at(p, 68)))
                              if p else None)
        out['Transforms'] = transforms

        # A partial range, which is where the two could disagree about whether
        # FirstBone indexes the pose or only the skeleton.
        if bone_count >= 3:
            partial = self.dll.GrannyNewLocalPose(bone_count)
            self.dll.GrannySampleModelAnimations(instance, 1, 2, partial)
            out['Partial'] = []
            for i in range(min(bone_count, 4)):
                p = self.dll.GrannyGetLocalPoseTransform(partial, i)
                out['Partial'].append(list(struct.unpack('<I16f', C.string_at(p, 68)))
                                      if p else None)
            self.dll.GrannyFreeLocalPose(partial)

            # And a range that does not fit, which must leave the pose alone.
            refused = self.dll.GrannyNewLocalPose(bone_count)
            self.dll.GrannySampleModelAnimations(instance, 0, bone_count + 1, refused)
            p = self.dll.GrannyGetLocalPoseTransform(refused, 0)
            out['Refused'] = (list(struct.unpack('<I16f', C.string_at(p, 68)))
                              if p else None)
            self.dll.GrannyFreeLocalPose(refused)

        # The world pose, which is where the skeleton hierarchy actually gets
        # walked. An offset that is not the identity, so that its placement in the
        # chain is checked rather than cancelling out.
        offset = (c_float * 16)(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 7, 11, 13, 1)
        world = self.dll.GrannyNewWorldPose(bone_count)
        if world:
            skeleton_ptr = C.cast(model.Skeleton, c_void_p)
            self.dll.GrannyBuildWorldPose(skeleton_ptr, 0, bone_count, pose,
                                          offset, world)
            out['World'] = []
            out['Composite'] = []
            for i in range(min(bone_count, 48)):
                p = self.dll.GrannyGetWorldPose4x4(world, i)
                out['World'].append(list(struct.unpack('<16f', C.string_at(p, 64)))
                                    if p else None)
                p = self.dll.GrannyGetWorldPoseComposite4x4(world, i)
                out['Composite'].append(list(struct.unpack('<16f', C.string_at(p, 64)))
                                        if p else None)
            # Past the end must be null, which CAddBoneFilter::Recalc depends on.
            out['PastTheEnd'] = self.dll.GrannyGetWorldPose4x4(world, bone_count) is None
            self.dll.GrannyFreeWorldPose(world)

        # A world pose smaller than the skeleton, which is exactly what
        # CAddBoneFilter allocates: nAddBone + 1 entries for a full skeleton.
        if bone_count >= 2:
            small = self.dll.GrannyNewWorldPose(bone_count - 1)
            if small:
                self.dll.GrannyBuildWorldPose(C.cast(model.Skeleton, c_void_p), 0,
                                              bone_count - 1, pose, offset, small)
                p = self.dll.GrannyGetWorldPose4x4(small, bone_count - 2)
                out['SmallWorldLast'] = (list(struct.unpack('<16f', C.string_at(p, 64)))
                                         if p else None)
                self.dll.GrannyFreeWorldPose(small)

        self.dll.GrannyFreeLocalPose(pose)
        self.dll.GrannyFreeModelInstance(instance)
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


def _group_duration(info, group_index):
    """The duration of the first animation that reaches this track group.

    A looping curve's period is the animation's duration, and a track group has
    no duration of its own. Every multi-knot curve in the corpus ends exactly at
    it, in all 134,098 of them, which is what makes the last control and the
    first the same keyframe.
    """
    target = C.addressof(info.TrackGroups[group_index].contents)
    for i in range(info.AnimationCount):
        a = info.Animations[i].contents
        for j in range(a.TrackGroupCount):
            if C.addressof(a.TrackGroups[j].contents) == target:
                return a.Duration
    return 0.0


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
    """Every place the two disagree, as (path, reference, candidate, relative).

    `relative` and `absolute` are the two ways two numbers can differ, and None
    for anything that is not a number. Both are needed. Relative alone is
    meaningless near zero, and a skinning matrix is full of entries that should be
    zero and come out at 1e-8 because a bind pose is undone by its own inverse:
    comparing those relatively reports a 20% difference between two numbers that
    are both noise. Absolute alone is meaningless for a translation of a thousand
    units.

    docs/GrannyReplacement.md sets the policy this implements: structural data
    compares exactly, float data compares with a tolerance reported as a metric.
    That distinction is not academic. Container and codec output is byte exact or
    wrong, and so is everything derived from it by copying. Matrix composition is
    float arithmetic, where two correct implementations differ in the last bit
    because they multiply in a different order, and the real DLL is not always the
    more accurate of the two.
    """
    out = []
    if isinstance(a, dict) and isinstance(b, dict):
        for key in sorted(set(a) | set(b)):
            if key not in a:
                out.append(('%s.%s' % (path, key), 'absent', 'present', None, None))
            elif key not in b:
                out.append(('%s.%s' % (path, key), 'present', 'absent', None, None))
            else:
                out += differences(a[key], b[key], '%s.%s' % (path, key))
    elif isinstance(a, list) and isinstance(b, list):
        if len(a) != len(b):
            out.append((path + '[]', '%d entries' % len(a), '%d entries' % len(b), None,
                        None))
        else:
            for i, (x, y) in enumerate(zip(a, b)):
                out += differences(x, y, '%s[%d]' % (path, i))
    elif isinstance(a, bool) or isinstance(b, bool):
        if a is not b:
            out.append((path, a, b, None, None))
    elif isinstance(a, (int, float)) and isinstance(b, (int, float)):
        # As float32, which is what both sides hold. Comparing the decimal forms
        # would report differences that are not there.
        x, y = _f32(a), _f32(b)
        if x != y:
            scale = max(abs(x), abs(y), 1e-30)
            out.append((path, a, b, abs(x - y) / scale, abs(x - y)))
    elif a != b:
        out.append((path, a, b, None, None))
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


def _init_worker(reference, candidate, tolerance):
    _WORKER['reference'] = Granny(reference)
    _WORKER['candidate'] = Granny(candidate)
    _WORKER['tolerance'] = tolerance
    _WORKER['transforms'] = differences(_WORKER['reference'].transforms(),
                                        _WORKER['candidate'].transforms(),
                                        'transforms')


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

    found = differences(want, got) + _WORKER['transforms']
    if not found:
        return label, 'same', found
    if all(d[3] is not None for d in found):
        # Rounding when the gap fits inside an absolute floor plus a relative
        # allowance, which is the usual form and the only one that behaves at
        # both ends of the scale.
        tolerance = _WORKER['tolerance']
        if all(d[4] <= tolerance + tolerance * max(abs(_f32(d[1])), abs(_f32(d[2])))
               for d in found):
            return label, 'near', found
    return label, 'differs', found


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
    parser.add_argument('--tolerance', type=float, default=1e-5,
                        help='float gap counted as rounding rather than as a wrong '
                             'answer, used as both an absolute floor and a relative '
                             'allowance (default 1e-5, about eighty float32 ulps at '
                             'magnitude one)')
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
    rounding = {}

    if args.jobs > 1:
        pool = multiprocessing.Pool(args.jobs, _init_worker,
                                    (args.reference, args.candidate, args.tolerance))
        results = pool.imap_unordered(_compare_one, unique(), chunksize=8)
    else:
        _init_worker(args.reference, args.candidate, args.tolerance)
        results = (_compare_one(item) for item in unique())

    total = 0
    for label, outcome, found in results:
        total += 1
        outcomes[outcome] += 1
        if outcome == 'differs':
            differing.append((label, found))
            for path, want, got, _relative, _absolute in found:
                key = generalise(path)
                histogram[key] += 1
                examples.setdefault(key, (label, want, got))
        elif outcome == 'near':
            for path, want, got, relative, absolute in found:
                key = generalise(path)
                worst = rounding.get(key)
                if worst is None or absolute > worst[1]:
                    rounding[key] = (relative, absolute, label, want, got)
        if total % 2000 == 0:
            sys.stderr.write('  %d files...\n' % total)

    if args.jobs > 1:
        pool.close()
        pool.join()

    print('%d unique GR2 files\n' % total)
    for outcome, count in outcomes.most_common():
        print('  %-28s %6d  %5.1f%%' % (outcome, count, 100.0 * count / max(total, 1)))

    if rounding:
        print('\nfields that differ only by rounding, worst gap:')
        for key, (relative, absolute, label, want, got) in sorted(
                rounding.items(), key=lambda kv: -kv[1][1]):
            print('  %-44s %.3g absolute, %.3g relative' % (key, absolute, relative))
            print('      e.g. %s: reference %r, candidate %r' % (label, want, got))

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
            for path, want, got, relative, absolute in found[:4]:
                print('      %s: %r vs %r%s'
                      % (path, want, got,
                         '' if relative is None
                         else '  (%.3g absolute, %.3g relative)' % (absolute, relative)))
            if len(found) > 4:
                print('      ... %d more' % (len(found) - 4))

    if args.report:
        with io.open(args.report, 'w', encoding='utf-8') as f:
            json.dump({'outcomes': dict(outcomes),
                       'histogram': dict(histogram),
                       'rounding': {k: {'relative': v[0], 'absolute': v[1],
                                        'file': v[2], 'reference': v[3],
                                        'candidate': v[4]}
                                    for k, v in rounding.items()},
                       'differences': [{'file': label,
                                        'differences': [{'path': p, 'reference': w,
                                                         'candidate': g,
                                                         'relative': r,
                                                         'absolute': s}
                                                        for p, w, g, r, s in found]}
                                       for label, found in differing]},
                      f, indent=1, default=str)
        print('\nwrote %s' % args.report)

    return 1 if outcomes['differs'] or outcomes['candidate-crashed'] else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
