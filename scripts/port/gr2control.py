"""Replay control sequences against two granny2 implementations and compare.

The playback layer is stateful, so a single call and a comparison says nothing
about it: a control carries a clock that advances, a weight that eases, and a
completion that latches. What does say something is a script. Bind a clip the way
CSkeletonAnimator binds one, set the same parameters, advance the model clock
forwards only, and read the clock, the weight, the completion and the resulting
pose after every step. Both implementations get the identical script and the two
transcripts are compared.

    python gr2control.py                     every animation in the corpus
    python gr2control.py --limit 200         a sample
    python gr2control.py --show 10           print more of what differs

This is the companion to gr2diff.py. That one compares what a file *contains*,
which is a pure function of the bytes; this one compares what an implementation
*does over time*, which is not. Together they are what stands in for a unit test
against the real DLL, which cannot exist: it needs the DLL and it needs 21,720
files of Nival's copyrighted data.

The nineteen scenarios below are the ones the engine can produce plus the ones it
cannot but a caller might: loop counts, speeds, a speed rewritten between frames
on a clock that is already hours old, a start time in the future, a completion
time, a deactivation partway through, both ease curve forms, and two clips
blended against each other.

Two comparisons are deliberately loose, and the reasons are in
docs/GrannyReplacement.md under "What still differs":

- Quaternions compare as rotations rather than as four numbers, since q and -q
  are the same rotation and nothing downstream can tell them apart. How often
  the two disagree about the sign is counted and reported rather than hidden.
- Floats compare with an absolute floor beside the relative allowance, because a
  relative difference is meaningless near zero.
"""

import argparse
import ctypes as C
import hashlib
import os
import sys
from collections import Counter
from ctypes import POINTER, c_bool, c_char_p, c_float, c_int32, c_void_p

# Beside this file, so running it from anywhere finds it.
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gr2diff
# gr2diff already declares every one of these structures with its pointers typed,
# and it is the file the corpus comparison uses, so a second declaration here
# would be a second thing to keep right.
from gr2diff import DEFAULT_CANDIDATE, DEFAULT_REFERENCE, FileInfo, Transform, _s as s

DLL = DEFAULT_REFERENCE


def bind(dll):
    dll.GrannyReadEntireFileFromMemory.argtypes = [c_int32, c_void_p]
    dll.GrannyReadEntireFileFromMemory.restype = c_void_p
    dll.GrannyGetFileInfo.argtypes = [c_void_p]
    dll.GrannyGetFileInfo.restype = c_void_p
    dll.GrannyFreeFile.argtypes = [c_void_p]

    dll.GrannyInstantiateModel.argtypes = [c_void_p]
    dll.GrannyInstantiateModel.restype = c_void_p
    dll.GrannyFreeModelInstance.argtypes = [c_void_p]
    dll.GrannySetModelClock.argtypes = [c_void_p, c_float]
    dll.GrannyNewLocalPose.argtypes = [c_int32]
    dll.GrannyNewLocalPose.restype = c_void_p
    dll.GrannyFreeLocalPose.argtypes = [c_void_p]
    dll.GrannyGetLocalPoseTransform.argtypes = [c_void_p, c_int32]
    dll.GrannyGetLocalPoseTransform.restype = POINTER(Transform)
    dll.GrannySampleModelAnimations.argtypes = [c_void_p, c_int32, c_int32, c_void_p]

    dll.GrannyBeginControlledAnimation.argtypes = [c_float, c_void_p]
    dll.GrannyBeginControlledAnimation.restype = c_void_p
    dll.GrannySetTrackGroupTarget.argtypes = [c_void_p, c_int32, c_void_p]
    dll.GrannySetTrackGroupAccumulation.argtypes = [c_void_p, c_int32, c_int32]
    dll.GrannyNewTrackMask.argtypes = [c_float, c_int32]
    dll.GrannyNewTrackMask.restype = c_void_p
    dll.GrannySetSkeletonTrackMaskFromTrackGroup.argtypes = [
        c_void_p, c_void_p, c_void_p, c_float, c_float, c_float]
    dll.GrannySetTrackGroupModelMask.argtypes = [c_void_p, c_int32, c_void_p]
    dll.GrannyEndControlledAnimation.argtypes = [c_void_p]
    dll.GrannyEndControlledAnimation.restype = c_void_p

    dll.GrannyFreeControl.argtypes = [c_void_p]
    dll.GrannyFreeControlOnceUnused.argtypes = [c_void_p]
    dll.GrannyControlIsComplete.argtypes = [c_void_p]
    dll.GrannyControlIsComplete.restype = c_bool
    dll.GrannyCompleteControlAt.argtypes = [c_void_p, c_float]
    dll.GrannySetControlActive.argtypes = [c_void_p, c_bool]
    dll.GrannyGetControlClampedLocalClock.argtypes = [c_void_p]
    dll.GrannyGetControlClampedLocalClock.restype = c_float
    dll.GrannySetControlRawLocalClock.argtypes = [c_void_p, c_float]
    dll.GrannyGetControlDuration.argtypes = [c_void_p]
    dll.GrannyGetControlDuration.restype = c_float
    dll.GrannyGetControlDurationLeft.argtypes = [c_void_p]
    dll.GrannyGetControlDurationLeft.restype = c_float
    dll.GrannyGetControlEffectiveWeight.argtypes = [c_void_p]
    dll.GrannyGetControlEffectiveWeight.restype = c_float
    dll.GrannyGetControlSpeed.argtypes = [c_void_p]
    dll.GrannyGetControlSpeed.restype = c_float
    dll.GrannySetControlSpeed.argtypes = [c_void_p, c_float]
    dll.GrannySetControlLoopCount.argtypes = [c_void_p, c_int32]
    dll.GrannySetControlForceClampedLooping.argtypes = [c_void_p, c_bool]
    dll.GrannyEaseControlIn.argtypes = [c_void_p, c_float, c_bool]
    dll.GrannyEaseControlIn.restype = c_float
    dll.GrannyEaseControlOut.argtypes = [c_void_p, c_float]
    dll.GrannyEaseControlOut.restype = c_float
    dll.GrannySetControlEaseIn.argtypes = [c_void_p, c_bool]
    dll.GrannySetControlEaseOut.argtypes = [c_void_p, c_bool]
    dll.GrannySetControlEaseInCurve.argtypes = [c_void_p] + [c_float] * 6
    dll.GrannySetControlEaseOutCurve.argtypes = [c_void_p] + [c_float] * 6
    return dll


class CScene(object):
    """One file, one model instance, and whatever controls a test binds to it."""

    def __init__(self, dll, path):
        self.dll = dll
        # A path, or the bytes themselves, so the corpus harness can hand over
        # what it already read out of a pak.
        self.data = path if isinstance(path, bytes) else open(path, 'rb').read()
        self.buf = C.create_string_buffer(self.data, len(self.data))
        self.handle = dll.GrannyReadEntireFileFromMemory(len(self.data),
                                                         C.cast(self.buf, c_void_p))
        self.info = C.cast(dll.GrannyGetFileInfo(self.handle),
                           POINTER(FileInfo)).contents
        self.model = self.info.Models[0]
        self.skeleton = C.cast(self.model.contents.Skeleton, c_void_p)
        self.bone_count = self.model.contents.Skeleton.contents.BoneCount
        self.animation = self.info.Animations[0]
        self.duration = self.animation.contents.Duration
        self.instance = dll.GrannyInstantiateModel(C.cast(self.model, c_void_p))
        self.pose = dll.GrannyNewLocalPose(self.bone_count)

    def bind(self, start_time=0.0):
        """The engine's seven calls, in the engine's order."""
        dll = self.dll
        builder = dll.GrannyBeginControlledAnimation(start_time,
                                                     C.cast(self.animation, c_void_p))
        if not builder:
            return None
        dll.GrannySetTrackGroupTarget(builder, 0, self.instance)
        dll.GrannySetTrackGroupAccumulation(builder, 0, 0)
        mask = dll.GrannyNewTrackMask(1.0, self.bone_count)
        group = C.cast(self.animation.contents.TrackGroups[0], c_void_p)
        dll.GrannySetSkeletonTrackMaskFromTrackGroup(mask, self.skeleton, group,
                                                     1.0, 1.0, 1.0)
        dll.GrannySetTrackGroupModelMask(builder, 0, mask)
        return dll.GrannyEndControlledAnimation(builder)

    def observe(self, control, clock):
        """Set the model clock, then read everything the engine reads."""
        dll = self.dll
        dll.GrannySetModelClock(self.instance, clock)
        return {
            'clock': clock,
            'complete': dll.GrannyControlIsComplete(control),
            'local': dll.GrannyGetControlClampedLocalClock(control),
            'weight': dll.GrannyGetControlEffectiveWeight(control),
            'left': dll.GrannyGetControlDurationLeft(control),
        }

    def sample(self, bone=0):
        self.dll.GrannySampleModelAnimations(self.instance, 0, self.bone_count,
                                             self.pose)
        t = self.dll.GrannyGetLocalPoseTransform(self.pose, bone)
        if not t:
            return None
        return (t.contents.Flags, list(t.contents.Position),
                list(t.contents.Orientation))

    def close(self):
        self.dll.GrannyFreeLocalPose(self.pose)
        self.dll.GrannyFreeModelInstance(self.instance)
        self.dll.GrannyFreeFile(self.handle)



# Each scenario is a list of steps. A step is ('set', name, args...) or
# ('clock', t): the first calls an entry point, the second advances the model
# clock, samples, and records everything readable.
def scenarios(d):
    fine = [i * d / 6.0 for i in range(19)]
    out = {}

    out['defaults'] = [('clock', t) for t in fine]

    for loops in (0, 1, 2):
        out['loops=%d' % loops] = ([('loop', loops)] + [('clock', t) for t in fine])

    out['loops=1,clamped'] = ([('loop', 1), ('clamped', True)]
                              + [('clock', t) for t in fine])
    for speed in (2.0, 0.5, 1.5):
        out['speed=%r' % speed] = ([('loop', 0), ('speed', speed)]
                                   + [('clock', t) for t in fine])

    # The infantry path, and the one shape none of the scenarios above had.
    # CMOUnitInfantry::AIUpdatePlacement calls SetSpeedFactorForAllAnimations on
    # every placement update of a moving soldier, so a looping run clip has its
    # speed rewritten between frames, on a model clock that is absolute game time
    # in seconds. The speed=N scenarios all set the speed before the clock has
    # moved at all, which is exactly the case a stateless
    # fmod((modelClock - startTime) * speed, d) gets right; this is the case it
    # gets wrong, by rescaling all the time already elapsed.
    walking = [('loop', 0)]
    for i, speed in enumerate([1.0, 1.0, 1.03, 1.03, 0.97, 1.0, 1.12, 0.9, 1.0]):
        walking.append(('speed', speed))
        walking.append(('clock', 300.0 + i * 0.04))
    out['speed-changes-while-walking'] = walking

    out['complete-at'] = ([('loop', 0), ('completeat', 1.5 * d)]
                          + [('clock', t) for t in fine])
    out['deactivate-midway'] = ([('loop', 0)] + [('clock', t) for t in fine[:8]]
                                + [('active', False)]
                                + [('clock', t) for t in fine[8:]])
    out['free-once-unused'] = ([('loop', 1), ('clamped', True), ('freeunused',)]
                               + [('clock', t) for t in fine])

    # The engine's own ease calls, from AddAnimationInternal.
    half = 0.25 * d
    out['engine-ease'] = ([('loop', 0), ('easein', True),
                           ('easeincurve', -half, half, 0.0, 0.0, 1.0, 1.0),
                           ('easeout', True),
                           ('easeoutcurve', 2 * d - half, 2 * d + half, 1.0, 1.0, 0.0,
                            0.0)]
                          + [('clock', t) for t in fine])
    out['ease-to-half'] = ([('loop', 0), ('easein', True),
                            ('easeincurve', 0.0, d, 0.0, 0.0, 1.0, 0.5)]
                           + [('clock', t) for t in fine])
    out['ease-control-in'] = ([('loop', 0), ('clock', 0.0), ('easecontrolin', 0.5 * d)]
                              + [('clock', t) for t in fine])
    out['ease-control-out'] = ([('loop', 0), ('clock', 0.0), ('easecontrolout', 0.5 * d)]
                               + [('clock', t) for t in fine])
    # The clock only ever moves forwards, because game time does. An earlier
    # version of this swept back to zero after setting the raw clock, and the two
    # implementations disagreed about a situation the engine cannot produce.
    out['raw-local-clock'] = ([('loop', 0), ('clock', 0.5 * d),
                               ('rawclock', 0.25 * d)]
                              + [('clock', 0.5 * d + t) for t in fine])
    out['start-time'] = [('clock', t) for t in fine]  # start time varies, see run()
    out['two-clips'] = None  # handled separately
    return out


def run_scenario(dll, data, steps, start_time=0.0, bones=(0, 1, 2)):
    scene = CScene(dll, data)
    control = scene.bind(start_time)
    if not control:
        scene.close()
        return None
    transcript = []
    for step in steps:
        kind = step[0]
        if kind == 'clock':
            dll.GrannySetModelClock(scene.instance, step[1])
            dll.GrannySampleModelAnimations(scene.instance, 0, scene.bone_count,
                                            scene.pose)
            row = {
                't': step[1],
                'complete': bool(dll.GrannyControlIsComplete(control)),
                'local': dll.GrannyGetControlClampedLocalClock(control),
                'weight': dll.GrannyGetControlEffectiveWeight(control),
                'duration': dll.GrannyGetControlDuration(control),
                'left': dll.GrannyGetControlDurationLeft(control),
                'speed': dll.GrannyGetControlSpeed(control),
                'pose': [],
                'quat': [],
            }
            for b in bones:
                if b < scene.bone_count:
                    f, p, o = scene.sample(b)
                    # The quaternion is kept apart from the rest so that it
                    # can be compared as a rotation. q and -q are the same
                    # rotation, and every consumer of a local pose is invariant
                    # to the sign: the matrix build is quadratic in it and the
                    # Hamilton product is bilinear.
                    row['pose'].append([f] + [round(x, 7) for x in p])
                    row['quat'].append([round(x, 7) for x in o])
            transcript.append(row)
        elif kind == 'loop':
            dll.GrannySetControlLoopCount(control, step[1])
        elif kind == 'clamped':
            dll.GrannySetControlForceClampedLooping(control, step[1])
        elif kind == 'speed':
            dll.GrannySetControlSpeed(control, step[1])
        elif kind == 'completeat':
            dll.GrannyCompleteControlAt(control, step[1])
        elif kind == 'active':
            dll.GrannySetControlActive(control, step[1])
        elif kind == 'freeunused':
            dll.GrannyFreeControlOnceUnused(control)
        elif kind == 'easein':
            dll.GrannySetControlEaseIn(control, step[1])
        elif kind == 'easeout':
            dll.GrannySetControlEaseOut(control, step[1])
        elif kind == 'easeincurve':
            dll.GrannySetControlEaseInCurve(control, *step[1:])
        elif kind == 'easeoutcurve':
            dll.GrannySetControlEaseOutCurve(control, *step[1:])
        elif kind == 'easecontrolin':
            transcript.append({'returned': dll.GrannyEaseControlIn(control, step[1],
                                                                   False)})
        elif kind == 'easecontrolout':
            transcript.append({'returned': dll.GrannyEaseControlOut(control, step[1])})
        elif kind == 'rawclock':
            dll.GrannySetControlRawLocalClock(control, step[1])
    scene.close()
    return transcript


def run_two_clips(dll, data, d):
    """Two clips on one instance, at different weights and different clocks."""
    scene = CScene(dll, data)
    a = scene.bind(0.0)
    b = scene.bind(0.0)
    if not a or not b:
        scene.close()
        return None
    dll.GrannySetControlLoopCount(a, 0)
    dll.GrannySetControlLoopCount(b, 0)
    dll.GrannySetControlEaseIn(a, True)
    dll.GrannySetControlEaseIn(b, True)
    transcript = []
    for wa, wb in ((1.0, 0.0), (0.0, 1.0), (0.5, 0.5), (0.75, 0.25), (0.2, 0.6),
                   (0.0, 0.0)):
        dll.GrannySetControlEaseInCurve(a, -1.0, 1e6, wa, wa, wa, wa)
        dll.GrannySetControlEaseInCurve(b, -1.0, 1e6, wb, wb, wb, wb)
        dll.GrannySetControlRawLocalClock(a, 0.0)
        dll.GrannySetControlRawLocalClock(b, 0.5 * d)
        dll.GrannySetModelClock(scene.instance, 0.25 * d)
        dll.GrannySampleModelAnimations(scene.instance, 0, scene.bone_count, scene.pose)
        row = {'wa': dll.GrannyGetControlEffectiveWeight(a),
               'wb': dll.GrannyGetControlEffectiveWeight(b), 'pose': [], 'quat': []}
        for bone in range(min(scene.bone_count, 6)):
            f, p, o = scene.sample(bone)
            row['pose'].append([f] + [round(x, 7) for x in p])
            row['quat'].append([round(x, 7) for x in o])
        transcript.append(row)
    scene.close()
    return transcript


#! How many sampled rotations agreed only after allowing for the sign.
SIGN_FLIPS = [0, 0]


def compare_quaternions(a, b, tolerance, path):
    """Compare two lists of quaternions as rotations rather than as four numbers.

    q and -q are the same rotation, and nothing downstream can tell them apart:
    building a matrix from one is quadratic in its components and composing two
    with GrannyPostMultiplyBy is bilinear. So a sign difference is a difference
    of representation, and comparing the four numbers directly would report it as
    a wrong answer. It is counted instead.
    """
    out = []
    for i, (x, y) in enumerate(zip(a, b)):
        SIGN_FLIPS[1] += 1
        direct = max(abs(u - v) for u, v in zip(x, y))
        flipped = max(abs(u + v) for u, v in zip(x, y))
        if flipped < direct:
            SIGN_FLIPS[0] += 1
            direct = flipped
        if direct > tolerance + tolerance * max(max(map(abs, x)), max(map(abs, y))):
            out.append((path + '[]', x, y, direct))
    return out


def compare(a, b, tolerance, path=''):
    """Differences between two transcripts, as (path, ref, cand, absolute)."""
    out = []
    if isinstance(a, dict) and isinstance(b, dict):
        for k in sorted(set(a) | set(b)):
            if k not in a or k not in b:
                out.append((path + '.' + k, k in a, k in b, None))
            elif k == 'quat':
                out += compare_quaternions(a[k], b[k], tolerance, path + '.' + k)
            else:
                out += compare(a[k], b[k], tolerance, path + '.' + k)
    elif isinstance(a, list) and isinstance(b, list):
        if len(a) != len(b):
            out.append((path + '[]', len(a), len(b), None))
        else:
            for i, (x, y) in enumerate(zip(a, b)):
                out += compare(x, y, tolerance, path + '[]')
    elif isinstance(a, bool) or isinstance(b, bool):
        if a is not b:
            out.append((path, a, b, None))
    elif isinstance(a, (int, float)) and isinstance(b, (int, float)):
        # The endless-duration sentinel is a magnitude where a relative test is
        # the only sensible one; everything else is seconds or a weight.
        gap = abs(float(a) - float(b))
        if gap > tolerance + tolerance * max(abs(a), abs(b)):
            out.append((path, a, b, gap))
    elif a != b:
        out.append((path, a, b, None))
    return out


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--limit', type=int, default=200)
    parser.add_argument('--tolerance', type=float, default=1e-5)
    parser.add_argument('--reference', default=gr2diff.DEFAULT_REFERENCE)
    parser.add_argument('--candidate', default=gr2diff.DEFAULT_CANDIDATE)
    parser.add_argument('--show', type=int, default=6)
    args = parser.parse_args()

    ref = bind(C.CDLL(os.path.abspath(args.reference)))
    cand = bind(C.CDLL(os.path.abspath(args.candidate)))

    seen = set()
    files = 0
    agree = 0
    histogram = Counter()
    examples = {}
    shown = 0

    for label, data in gr2diff.iter_corpus(gr2diff.DEFAULT_CORPUS, '*Animations*'):
        digest = hashlib.sha1(data).digest()
        if digest in seen:
            continue
        seen.add(digest)

        # Only files with an animation and a model to bind it to.
        scene = None
        try:
            scene = CScene(ref, data)
            if scene.info.AnimationCount == 0 or scene.info.ModelCount == 0:
                continue
            d = scene.duration
            if not (d > 0.0):
                continue
        except Exception:
            continue
        finally:
            if scene is not None:
                scene.close()

        files += 1
        if files > args.limit:
            break
        if files % 100 == 0:
            print('  %d files...' % files, file=sys.stderr)

        differences = []
        for name, steps in scenarios(d).items():
            if steps is None:
                a = run_two_clips(ref, data, d)
                b = run_two_clips(cand, data, d)
            else:
                start = 0.5 * d if name == 'start-time' else 0.0
                a = run_scenario(ref, data, steps, start)
                b = run_scenario(cand, data, steps, start)
            if a is None or b is None:
                if (a is None) != (b is None):
                    differences.append(('%s: bound' % name, a is not None,
                                        b is not None, None))
                continue
            differences += [('%s%s' % (name, p), x, y, g)
                            for p, x, y, g in compare(a, b, args.tolerance)]

        if not differences:
            agree += 1
        else:
            for p, x, y, g in differences:
                key = p.split('[')[0]
                histogram[key] += 1
                examples.setdefault(key, (label, x, y, g))
            if shown < args.show:
                shown += 1
                print('\n%s' % label)
                for p, x, y, g in differences[:6]:
                    print('    %s: %r vs %r%s'
                          % (p, x, y, '' if g is None else '  (%.3g)' % g))

    print('\n%d files with an animation and a model' % files)
    print('  agree      %6d  %5.1f%%' % (agree, 100.0 * agree / max(files, 1)))
    print('  differ     %6d  %5.1f%%' % (files - agree,
                                          100.0 * (files - agree) / max(files, 1)))
    print('  sampled rotations that matched only after allowing for the sign: '
          '%d of %d (%.2f%%)'
          % (SIGN_FLIPS[0], SIGN_FLIPS[1],
             100.0 * SIGN_FLIPS[0] / max(SIGN_FLIPS[1], 1)))
    if histogram:
        print('\nwhat differs:')
        for key, count in histogram.most_common(20):
            label, x, y, g = examples[key]
            print('  %-52s %6d' % (key, count))
            print('      e.g. %s: %r vs %r' % (label, x, y))


if __name__ == '__main__':
    main()
