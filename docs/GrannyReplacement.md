# Replacing Granny 3D: findings, evidence and plan

> **Provenance.** Everything below was measured on 2026-08-27 against the game data
> installed on this machine, not inferred from documentation, except [Which
> granny2.dll](#which-granny2dll-and-what-changed-since-the-game-shipped), measured
> 2026-08-28 against the DLL copies and SDK archives on this machine. Where a number
> is quoted, the method that produced it is given in [Reproducing the
> measurements](#reproducing-the-measurements). Companion document:
> [Granny3DUsage.md](Granny3DUsage.md) describes how the engine *uses* Granny; this
> one describes how to *replace* it.

## Summary

Replacing Granny in this engine is a smaller and much lower risk job than it looks,
because the reverse engineering has already been done by other projects and is now
verified correct against RAD's own DLL on this game's assets.

- The game uses **exactly one GR2 dialect**: File Format 6, little endian, 32-bit
  pointers, `fileInfoSize` 56. No BitKnit, no big endian, no 64-bit files, in any
  corpus including mods.
- Two independent clean-room decoders (`blendergranny`, Python; `granny-ro-js`, JS)
  produce **byte-identical output to `granny2.dll`** on this game's data.
- The animation curves are the **pre-`curve2` legacy layout** everywhere, which
  makes the sampler far smaller than any reference library implements.
- **Animation is presentation-only.** It never reaches `AILogic`, so a replacement
  does not need bit-exactness with the DLL, only visual correctness.
- The genuinely unsolved part is the **playback and blending layer** (roughly 30 of
  the 54 entry points), which no open source project implements, because importers
  and viewers never need it.
- The engine no longer targets the Granny the game shipped with. It was moved from
  **2.5.0.5** to **2.11.8.0**, and eight of the structures it walks changed shape
  between the two. A replacement reproduces 2.11's layouts. See
  [Which granny2.dll](#which-granny2dll-and-what-changed-since-the-game-shipped).

Estimate for a working replacement: **27 to 44 working days**, see [Plan](#plan).

## The corpora

Four bodies of data were surveyed. `Versions/Current/Data/` in this repository is a
**pre-release beta snapshot and is not representative of the shipped game**, since
its compression mix is inverted relative to retail. Measure against the installed
games.

| corpus | location | paks | unique GR2 |
|---|---|---|---|
| repo beta | `Versions/Current/Data/bin/` | loose files | 15,742 |
| retail base | `C:\Games\bk2` | 30 | 15,678 |
| Fall of the Reich | `C:\Games\BK2-FoTR` | 29 | 15,692 |
| Total Conversion | `C:\Games\Blitzkrieg 2 -  Total Conversion` | 250 | 51,814 |

Universal MOD-18 and MPBMod6 are installed under `mods/` in both the base game and
FoTR, so they are included in those counts. Total across the three installs:
**83,184 unique GR2 files, 15.5 GB of paks.**

**Careful with that number: it is a sum of per-install counts, so a file present
in all three is counted three times.** Deduplicated by content across the three,
`scripts/port/gr2diff.py` finds **21,720 distinct GR2 files**, of which 15,704 are
Oodle1 and 6,016 Oodle0. The larger figure is the right one for "how much data is
out there"; the smaller one is the right one for "how much has to be validated",
and the difference is nearly four to one.

### `.pak` files are ZIP archives

They begin with `PK\x03\x04`. Some entries are stored, some deflated. Python's
`zipfile` reads them directly with no format reversing required, which is how every
census below was taken. GR2 resources live under `bin/Geometries/`,
`bin/Animations/`, `bin/Skeletons/`, `bin/AIGeometries/` with extensionless GUID
names.

### Compression, per file

| corpus | Oodle0 | Oodle1 | other |
|---|---|---|---|
| repo beta | 14,113 (89.7%) | 1,629 (10.3%) | none |
| retail base | 6,015 (38.4%) | 9,662 (61.6%) | 1 file mixed `none`+`oodle0` |
| Total Conversion | 6,012 (11.6%) | 45,798 (88.4%) | 3 uncompressed, 1 mixed |

Two consequences for an implementation:

1. **Both codecs are mandatory.** Oodle1 is the majority path in shipped data;
   Oodle0 still covers 38% of the retail game.
2. **Dispatch per section, not per file.** Mixed-compression files exist. They are
   vanishingly rare (one file), which is exactly why the assumption is dangerous.

### Non-GR2 files live in the GR2 directories

A handful of entries under `bin/Geometries/` and friends have other magics, for
example `636F4C5B 7A696C61` (ASCII `[Localiz...`). The loader **must reject a bad
magic gracefully**. Per `port/PORT_ROADMAP.md` a failed resource load currently
crashes, so this is a live hazard rather than a theoretical one.

## The format, as it actually appears in this game

### One dialect, four struct tags

Every GR2 in every corpus: magic `CAB067B8 0FB16DF8 7E8C7284 1E00195E` (File Format
6, LE, 32-bit pointers), `format` 6, `fileInfoSize` 56.

The `tag` field at offset 68 identifies the structure version:

| tag | beta | base | fotr | tc |
|---|---|---|---|---|
| `0x8000000F` | 484 | 315 | 310 | 312 |
| `0x80000010` | 13,883 | 5,944 | 5,930 | 5,948 |
| `0x80000011` | - | - | - | 3 |
| `0x80000013` | 1,375 | 9,419 | 9,452 | 45,551 |

Four tags in the wild, one of which appears only in mod content. **Resolve struct
members by name through the file's own type section rather than by hardcoded
offset.** This is the single most important parser design decision: it is why
`blendergranny` reads these files correctly and why `nwn2mdk`, which hardcodes
offsets for NWN2's `0x80000015`, reads skeletons correctly but produces garbage
transform tracks.

### Granny structs are packed

The in-memory structures have **no padding**. `granny211.h` states this indirectly
via assertions of the form `GrannyTypeSizeCheck(sizeof(granny_file_info) ==
sizeof(a) + sizeof(b) + ...)`. Reading them with natural alignment silently yields
plausible garbage: a first attempt here reported `SkeletonCount = 398` instead of 1.

Verified sizes with `_pack_ = 1` on x64:

| struct | size |
|---|---|
| `granny_variant` | 16 |
| `granny_transform` | 68 |
| `granny_bone` | 164 |
| `granny_skeleton` | 40 |
| `granny_model` | 112 |
| `granny_animation` | 56 |
| `granny_file_info` | 148 |

### The structures in the file are not the structures the engine reads

`GrannyGetFileInfo` cannot hand back a pointer into the loaded file. It has to
convert, because the two ends disagree about what these structures are.

`Versions/Temporary/Engine/Sources/vendor/granny/include/granny.h` is a two-line
shim that includes `granny211.h`, so the engine reads **2.11** layouts. The files
were written by a 2.5-era exporter and carry 2.5-era type trees. Dumped from a
shipped geometry file with `scripts/port/gr2info.py types`:

| in the file | in `granny211.h` |
|---|---|
| `Name`, `ParentIndex` | same |
| `Transform` | `LocalTransform` |
| `InverseWorldTransform` | `InverseWorld4x4` |
| `LightInfo`, reference | `LODError`, real32 |
| `CameraInfo`, reference | `ExtendedData` |
| `ExtendedData` | absent |

Seven members against six, and the ones that survived were renamed, so a
conversion cannot be driven by matching member names alone. The real DLL carries
its own type definitions as exported globals, `GrannyBoneType` and its siblings,
alongside `GrannyDataTypesAreEqual` and `GrannyDataTypeBeginsWith`; that is the
machinery it converts with. `GrannyOldCurveType` is exported too, which is how a
2017 build still reads the 20-byte curves below.

**The conversion was measured, not inferred.** Load a 134-bone skeleton through
`granny2_x64.dll`, take `Skeletons[0]->Bones`, and read the array at both
strides. At 164 bytes, the 2.11 layout, `ParentIndex` is a valid parent chain
(`-1, 0, 1, 0, 3, ...`) and all 134 names read; at 176, the file's layout on a
64-bit host, it is noise. Every bone also comes back with a `LODError`, a member
that appears nowhere in the file: the string is not in it at all. Granny is
synthesising it, and the value it synthesises is **0.0**, measured over 60 files
through a struct whose `sizeof` was asserted against `granny211.h` first. A
first attempt read it at offset 140 and reported 1.0, which is
`InverseWorld4x4[15]`, the bottom right of an identity matrix; `LODError` is at
144.

The other members the conversion has to invent, measured the same way:
`skeleton.LODType` 0, `animation.DefaultLoopCount` 0, `animation.Flags` 0, and
`ExtendedData` null on `file_info`, `model`, `skeleton`, `bone` and `animation`.
`animation.Oversampling` is 0.0 or 2.0, because two versions of that structure
appear in the corpus and only one of them carries the member.

So 2.11 does not misread these files. It converts them, which is what the
self-describing type tree is for, and it is load-bearing rather than lucky: the
engine indexes `Bones[i]`, so a wrong stride would break every bone after the
first. What the conversion drops costs nothing here, because `LightInfo` and
`CameraInfo` are null in all 2,472 bones of a 250-file sample, and no engine
code reads `LODError`.

Two different transformations are involved and it is worth keeping them apart.
`granny_file_info` has the *same members* in both, and differs only in
representation: 92 bytes against 148, because pointers double and each
`ArrayOfReferences` becomes a separate count and pointer. That is marshalling,
and it applies to every structure. `granny_bone` differs in its *members*. That
is a version change, and it applies only to the eight listed under "Which
`granny2.dll`" below.

**Consequence for M2.** The type tree walk produces objects in the file's shape,
and a second step lays them out in the shape `granny211.h` describes. The
mapping between the two is per type and has to be written by hand for the eight
structures that changed, which are listed under "Which `granny2.dll`" below.

### Curves are the pre-`curve2` legacy layout

This game predates `granny_curve2`. A transform track on disk is **64 bytes**:

```c
struct TransformTrack {          // 4 + 3*20 = 64 bytes
    char*    name;
    OldCurve position, orientation, scale_shear;
};
struct OldCurve {                // 20 bytes
    int32_t  degree;
    int32_t  knots_count;    float* knots;
    int32_t  controls_count; float* controls;
};
```

For contrast, `nwn2mdk` models the newer form at **28 bytes** (`{name, 3 x {keys*,
curve_data*}}`) with a format byte selecting among twelve compressed encodings.
Feeding this game's data through that layout yields a correct first track, then a
null name, then a crash. Confirmed by re-reading the same file with the 64-byte
layout and recovering all twelve track names cleanly.

### The exact curve matrix

Census over 4,500 animations and 148,415 curves (18% of base and FoTR animations, 5%
of TC's), plus a cross-tabulation over 1,200 animations:

```
codec: LegacyCurve32f, 100%.  No granny_curve2 variant appears anywhere.

pos  dim=3  degree=0   19540      pos  dim=3  degree=2    3832
rot  dim=4  degree=0    8466      rot  dim=4  degree=2    6717
scl  dim=9  degree=0    1511      scl  dim=9  degree=1      21
```

Six combinations, and the cross is deliberately not full:

- **Dimension is determined by the slot.** Position is always 3, orientation always
  4, scale-shear always 9. No dimension dispatch is needed.
- **Degree 1 occurs only on scale-shear**, and scale-shear is never degree 2.
- **74% of all curves are degree 0**, meaning constant. Only 26% interpolate at all,
  and the risk concentrates in the 6,717 degree-2 quaternion curves.

So the sampler needs:

```
position    (3-vector)   : constant  |  quadratic B-spline
orientation (quaternion) : constant  |  quadratic B-spline + neighbourhooding
scale-shear (9-vector)   : constant  |  linear
```

**Scale-shear is animated** in 2.0% (base) to 4.4% (TC) of curves, so the
`granny_transform.Flags` bits must be honoured; scale cannot be assumed identity.
`SceneB2/WingScaleMutator.cpp` exists for this reason.

### Three exporter vintages, over the whole corpus

The census above sampled. Converting the animation structures needed all 21,720
files, and the full pass found a **third** version of the track group that the
sample had missed. Every file's type tree, counted by member list:

| files | track group | animation | scalar track |
|---|---|---|---|
| 15,457 | `VectorTracks`, `TransformLODErrors`, `RootMotion` | `Oversampling` | `Dimension` |
| 5,948 | `ScalarTracks`, `RootMotion` | no `Oversampling` | no `Dimension` |
| 315 | `ScalarTracks` | no `Oversampling` | no `Dimension` |

The transform track, the curve, the text track, the text track entry and the
periodic loop are the **same shape in all 21,720**, and the curve is the
pre-`curve2` layout in every one of them: no file carries a `granny_curve2`
variant, so there is exactly one conversion to write and not eighteen.

Reading members by name is what lets one converter serve all three. Reading by
offset would need three tables and would silently produce plausible garbage on
whichever vintage it was not written for.

What the real DLL does with the members no file has, over 11,400 animated files,
11,360 track groups, 257,581 transform tracks and 772,743 curves:

- `animation.Oversampling` is read where the member exists (2.0 in 8,410, 1.0 in
  6) and **0.0** in the 2,984 animated files that lack it.
- `animation.DefaultLoopCount`, `animation.Flags`, `transform_track.Flags`,
  `vector_track.TrackKey` and `track_group.TransformLODErrorCount` are **0**
  everywhere, and `ExtendedData` and `PeriodicLoop` are null everywhere.
- `track_group.Flags` is the file's `AccumulationFlags` renamed: 2 in 11,189
  groups and 0 in 171.
- Every curve comes back **`DaK32fC32f`**, format byte 1, with `Degree`, `Knots`
  and `Controls` copied straight out of the old curve. `Padding` is left
  uninitialised: three curves of one file returned 16414, -17102 and 0.
- An animation's `TrackGroups` are the same objects as the file's, in all 11,360.

Two things the corpus cannot check, because it contains none: a **text track**
(zero in 21,720 files) and a **periodic loop** (null in every track group). Both
are converted from the type tree, which every file carries whether it uses it or
not, and covered only by authored fixtures in `test/Animation.cpp`. A vector
track is nearly as rare, 24 of them in 5 files, and all 24 are in the newest
vintage, so what the DLL puts in `Dimension` for a file that lacks the member was
not observable.

### What GrannyEvaluateCurveAtT computes

Measured, not derived. Setting one control to one and the rest to zero and
sweeping t reads `basis_i(t)` straight out of `granny2.dll`, which pins the
formula without any hypothesis about which spline RAD picked.

Degree 0 is constant and degree 1 is linear. Degree 2 is a **non-uniform**
quadratic B-spline, in span *i* with `u = (t - k[i]) / (k[i+1] - k[i])` and
`h = k[i+1] - k[i]`:

```
w-  =  h (1-u)^2 / ( k[i+1] - k[i-1] )      on control i-1
w+  =  h u^2     / ( k[i+2] - k[i]   )      on control i+1
w0  =  1 - w- - w+                          on control i
```

The trap is that a typical curve's later spans are evenly spaced, where this
reduces to the uniform basis `((1-u)²/2, (1+2u-2u²)/2, u²/2)`. A uniform
implementation therefore agrees on most samples of most curves and is wrong at
both ends and wherever a span's neighbour is a different length.

Off the ends the sequence is **clamped**: `k[-1] = k[0]` with `c[-1] = c[0]`, and
`k[n] = k[n-1]`. Reflecting the last span instead is the natural guess and is
wrong; it was caught by solving for the divisor the measured weights imply, which
came back as the last knot exactly.

**Looping** replaces the clamp with a wrap of period `CurveDuration`. Every
multi-knot curve in the corpus ends exactly at its animation's duration, in all
134,098 of them, so the last control is the same keyframe as the first and the
cycle has n-1 entries:

```
BackwardsLoop:  k[-1] = k[n-2] - CurveDuration,  c[-1] = c[n-2]
ForwardsLoop:   k[n]  = k[1]   + CurveDuration,  c[n-1] -> c[0]
```

At degree 1 only the control identification survives; a straight line needs no
knot outside its own span, and `BackwardsLoop` was measured to change nothing.

Three more things, each of which an implementation gets wrong by default:

- **`Normalize` does not reach the constant path**, nor the empty-curve identity
  path. A degree-0 control of length 5 comes back with length 5 even when
  normalization is asked for. Normalizing it disagrees with the DLL on 2,884
  files.
- **The stride is the caller's `Dimension`**, not the curve's. A curve stores a
  knot count and a control count and no dimension, and asking a three-wide curve
  for one component returns the first float of each *control*, not the first
  component of each key.
- **At `Dimension` 3 the DLL's `Normalize` is not a normalization.** It is
  deterministic and it does not write past the result, but the divisor it implies
  ranges from -10.6 to 47,000 times the vector's own length and sometimes flips
  the sign. There is no rule there to reproduce. The engine passes `Normalize`
  false, and the sampler only ever normalizes quaternions, so nothing reaches it.

Shapes the sampler has to survive, from a census of all 772,767 curves: 322,479
are empty; 316,190 are degree 0 and every one has exactly one knot; degree 1
never has fewer than 2 knots and degree 2 never fewer than 3; **38 curves have two
knots at the same time**, a zero-length span that divides by zero if unguarded;
and every non-empty curve starts at knot 0.0, which is why `t` below the first
knot never arises. The real DLL answers from an essentially arbitrary span there
— span 0 just below the first knot, span 23 at t = -1, the last span at t = -10 —
and that is not reproduced.

### The playback layer, measured

No open source project implements this, so all of it was measured by scripting
`granny2.dll` through the sequences `CSkeletonAnimator` issues, advancing the
model clock forwards only, and reading the observables after every step. The
harness replays fifteen scenarios per file against both implementations and
compares the transcripts; it is the record-and-replay rig this document asked for
before M4 rather than after.

The clock, with `raw = (modelClock - startTime) * speed` floored at zero:

| | |
|---|---|
| clamped local clock | `raw` modulo the duration, except that once `raw` reaches `loopCount` whole periods it stops at the duration instead of wrapping. A loop count of zero never stops |
| `GetControlDuration` | `loopCount * duration / abs(speed)`, and the bit pattern `0x7f0fffff` when the loop count is zero |
| `GetControlDurationLeft` | that duration minus `modelClock - startTime`, which is model time and not local time, and which goes negative |
| `IsComplete` | **only ever true after `CompleteControlAt`**, and only once the clock reaches the time it was given. A clip that has simply finished playing is not complete |
| `FreeControlOnceUnused` | no observable effect at all. The control keeps running and stays valid, which is what the engine depends on: it calls this on every clip with no end time and then goes on using the pointer |
| `ForceClampedLooping` | no observable effect either, at any loop count |

The **ease curves are cubic Beziers** over their four numbers in the Bernstein
basis, fitted over a grid rather than guessed: `(1,0,0,0)` evaluates to
`(1-u)^3`, `(0,0,0,1)` to `u^3`, and the engine's own `(0,0,1,1)` to
`3u^2 - 2u^3`. The four numbers are **stored as bytes**,
`floor(v * 255 + 0.5) / 255`, which is why asking for a weight of 0.5 gives back
0.501961, and 0.7 gives 0.701961 rather than the 0.698039 that rounding half to
even would produce. Outside its interval each curve is **one** on the side it is
not easing on and holds its endpoint on the other, so an ease-in that ends on
anything but one is discontinuous. The engine never does that.

**Sampling** blends the controls bound to an instance as a weighted average
divided by the total weight, **with a floor at 0.2**: below that total the
shortfall goes to the bone's rest pose, so a lone clip at weight 0.05 shows a
quarter of the way from the bind pose and one at 0.25 shows at full strength. The
floor is exactly 0.2, measured over 222 samples across 60 files, and it is easy
to miss: any measurement made with two controls whose weights sum past it sees
only the normalisation. The bind pose brings its own flags in with it when it
contributes. Where no control reaches a bone at all, the rest pose stands
untouched.

A track that mentions a bone **replaces every part of its transform**, and the
parts whose curves are empty become neutral rather than the bind pose: a bone
whose track carries an orientation curve and no position curve comes back at the
origin.

The curves are sampled with the loop flags set from the **loop index**, not the
loop count: a curve wraps backwards when there is a period before this one and
forwards when there is one after. So the first pass of a clip that plays twice
does not wrap at its start and the second does.

One thing the public curve entry point does not do, and the sampler must: at a
wrap the control brought in from the far end of the curve can be on the opposite
side of the quaternion sign ambiguity from the local ones. One bone in this
corpus has orientation keys whose `w` runs from +1.0007 at the first to -1.0035
at the last. Blended raw, the wrap frame produces a rotation unrelated to either
neighbour, which is a visible pop once per loop.

#### What still differs

Over 200 files and fifteen scenarios each, everything above agrees. Two things
do not, and neither is reachable in a way the game can see.

- **A local clock landing exactly on a multiple of the period**, where `fmod`
  gives zero and the DLL gives the period. A float knife edge in the wrap
  arithmetic. For a looping clip the two answers are the same pose, because the
  last key and the first are the same keyframe.
- **The sign of a sampled quaternion**, on 1.3% of samples, and the two-clip
  blends that inherit it. q and -q are the same rotation and nothing downstream
  can tell them apart: the matrix build is quadratic in the components and
  `GrannyPostMultiplyBy` is bilinear, so the world and composite matrices are
  identical either way. The DLL negates on bones whose rotation passes through
  180 degrees. Five rules were fitted and rejected, and the choice is not
  stateful; `granny_bound_transform_track` in `granny211.h` carries a
  `QuaternionMode` byte computed when a clip is bound, which is very likely where
  the decision lives and is not readable from outside.

A third difference, the frame where an ease-in's weight first became non-zero,
turned out to be the missing weight floor above and is gone.

## Library survey

Seven open source GR2 implementations were examined and, where possible, built and
run against this game's data.

| | opengr2 | MacLarian | nwn2mdk | Knit | blendergranny | granny-ro-js | noclip |
|---|---|---|---|---|---|---|---|
| language | C | Rust | C++ | C# | Python | JS | TypeScript |
| licence | MPL-2.0 | PolyForm-NC | Apache-2.0 | EUPL-1.2 | **MIT** | MIT | MIT |
| FF6 / our magic | yes | **no**, FF7 only | yes | yes | yes | yes | yes |
| Oodle0 | **corrupts heap** | no | no | no | **yes, verified** | **yes, verified** | **refuses** |
| Oodle1 | yes | no | yes | yes | **yes, verified** | out of scope | **refuses** |
| BitKnit | no | yes | no | yes | yes | no | no |
| structs for our tags | generic tree only | FF7 offsets | partial | yes | **yes** | yes | yes |
| meshes | no | yes (FF7) | **no** | yes | yes | yes | yes |
| animation | no | **no** | yes (`curve2`) | yes | yes | yes | yes |
| pose / skinning runtime | no | no | no | no | no | **yes** | **yes** |
| blending / controls | no | no | no | no | no | no | no |
| 64-bit host | yes | yes | **no, x86 only** | yes | yes | yes | yes |

Notes on each:

- **opengr2** (2,926 lines C). Generic type-tree parser, correct FF6 magic, and a
  sound virtual-pointer scheme for 32-bit files on 64-bit hosts. But `gr2_read.c:284`
  falls `COMPRESSION_TYPE_OODLE0` through into the Oodle1 decoder (the real Oodle0
  case is `#if 0`'d out), which produced access violations and heap corruption on
  60 of 60 Oodle0 files tested. No high-level structs, no animation. **Do not fork**:
  its element-tree abstraction is not what this engine needs, and roughly 90% of the
  work would remain.
- **MacLarian / MacPak** (Rust, BG3 tooling). Rejects these files twice over: its
  `LE32` magic is the File Format 7 constant, and it returns
  `"Oodle compression not supported"` for both Oodle codecs. No animation at all;
  its writer literally emits `// Animations (empty)`. Its GR2-to-glTF mapping is
  good *reference* for skeleton and vertex-attribute conversion. PolyForm-NC is not
  an open source licence.
- **nwn2mdk** (C++, Apache-2.0). Magic is byte-identical to ours; NWN2 and BK2 are
  the same Granny generation. Reads this game's skeletons and animations correctly
  (verified: `Basis`, 10 bones, `Basis` then `Turret` then `GunCarriage` then
  `MainBarrel`). Refuses Oodle0. Has no mesh structs, since NWN2 keeps meshes in MDB.
  Its `virtual_ptr` stores a `uint32_t` cast to `void*`, so the **x64 build crashes
  immediately** while x86 works. Best source for FF6 struct definitions and for an
  already-C++ Oodle1 decoder.
- **Knit** (C#, EUPL-1.2 with an MPL-2.0 Oodle file). The only project that
  *documents* Oodle0, describing it as "functionally the same as Oodle1" modulo
  endianness. **That claim does not hold for this game's files**: its decoder throws
  or emits zeros on them while succeeding on Oodle1. Presumably true of modern
  Granny, not of the 2002-era codec. Architecturally the cleanest design of the six,
  being type-definition driven. Requires a .NET 11 preview SDK for its CLI tools;
  the library itself builds on .NET 10.
- **blendergranny** (Python, MIT). **All four codecs**, meshes, skeletons,
  animations, our exact format version, and an explicit clean-room provenance policy
  in `docs/PROVENANCE.md`. The `io_scene_gr2/gr2/` package is Blender-independent
  and imports under plain CPython. **The reference implementation for M1 and M2.**
- **granny-ro-js** (JS, MIT). Scoped to precisely our dialect: "Granny format 6,
  little-endian, 32-bit pointers, Oodle0". Its Oodle0 is a port of blendergranny's.
  Uniquely, it implements a **pose runtime** validated against the real DLL, with
  the DLL's float quirks documented inline (fast one-Newton-step quaternion
  normalise, no second renormalise when building the bone matrix, f64 FK cascade).
  **The reference implementation for M3.** One diligence note: a comment in
  `GrannyOodle0.js` cites a leaked RAD source path as an "asm-cite oracle"; prefer
  blendergranny's upstream version, which carries the explicit no-leaked-source
  policy.
- **noclip.website** (TypeScript, MIT, `src/RagnarokOnline/granny*.ts`, about 1,430
  lines). A real-time renderer for the same Granny generation, so a second opinion on
  the pose path. **No decompression at all**: `granny.ts` throws on either Oodle
  codec, since it expects files pre-expanded offline by `tools/gr2_decompress.c`, a
  Win32 shim around the real DLL. Reads `degree` and then **ignores it**, sampling
  every curve as piecewise interpolation between the two bracketing control points:
  `vec3.lerp` for position, `quat.slerp` for orientation, component-wise lerp for
  scale-shear. Resolves members by name through the type tree throughout, and
  dispatches on the curve slot for dimension, independently confirming both design
  calls above. Its LICENSE notes that some implementations are reverse engineered and
  not clean-room, which matters if vendoring from it. Nothing here needs vendoring.

**Recommended sources:** blendergranny for codecs and parsing, nwn2mdk for FF6
struct definitions and an existing C++ Oodle1, granny-ro-js for pose, noclip for the
viewer-grade sampling fallback described below, opengr2 for the virtual-pointer idea
only (about 30 lines, read it, take nothing else).

## Which `granny2.dll`, and what changed since the game shipped

Measured 2026-08-28 over every copy on this machine. By SHA-256 there are **two**
distinct binaries, not five.

| version | size | PE link date | architectures | where |
|---|---|---|---|---|
| **2.5.0.5** | 400,951 | 2004-11-12 | **x86 only** | FoTR `bin_`, Total Conversion `Bin.original`, and `vendor.rar` |
| **2.11.8.0** | 672,256 / 803,328 | 2017-09-28 | x86 and x64 | `third_party/uesp-esoapps`, and `Granny_Common_2_11_8_0_Release.zip` |

Both rows are byte-identical within themselves, and `third_party`'s `granny211.h` is
byte-identical to the 2.11.8.0 SDK's `include/granny.h`. So the vendored Granny is
simply that SDK release, and **`vendor.rar` is the original Nival vendor SDK**: the
2.5 DLL the game shipped with, plus its `granny.h`, `gr2_viewer.exe`, `grn2gr2` and
`granny2.chm`.

**A minor version number covering a major break.** 682 exports become 836: 146
disappear, 300 arrive, and 51 of the 536 that survive change argument size. Among the
54 this engine uses, all 54 exist in both, and exactly one changed shape:
`GrannyEvaluateCurveAtT`, from five arguments taking a `granny_curve *` to nine
taking a `granny_curve2 *`.

The engine was moved to 2.11 in commit `62ff1e13e`, and that commit is the complete
list of what the move cost: the `GrannyEvaluateCurveAtT` call, the allocator callback
gaining `AllocationIntent` with `granny_uintaddrx` sizes, the opaque handles moving
into a `granny::` namespace so forward declarations had to change, and
`GetModelNameOfSkeleton` returning `const char *`.

**Eight structures changed layout**, which is what matters for M2 and M3. A wrong
offset is silent: the engine walks these by hand in twenty-odd places, so it reads
plausible garbage rather than failing.

| structure | 2.5 to 2.11 |
|---|---|
| `granny_bone` | 7 fields to 6: `LightInfo` and `CameraInfo` dropped, `LODError` added |
| `granny_skeleton` | 3 to 5: `LODType` and `ExtendedData` added |
| `granny_model` | 5 to 6: `ExtendedData` added |
| `granny_animation` | 6 to 9: `DefaultLoopCount`, `Flags`, `ExtendedData` added |
| `granny_track_group` | 15 to 14: `RootMotion` dropped |
| `granny_tri_topology` | 18 to 22: polygon index arrays added, `IndexCount16` renamed `Index16Count` |
| `granny_transform_track` | `granny_curve` becomes `granny_curve2`, `Flags` added |
| `granny_data_type_definition` | `TraversalID` (`granny_uint32`) becomes `Ignored_Ignored` (`granny_uintaddrx`), so its size differs between x86 and x64 |

Unchanged apart from `const` on name pointers: `granny_file_info`, `granny_mesh`,
`granny_vertex_data`, `granny_transform`, `granny_tri_material_group`,
`granny_bone_binding`.

Three consequences:

- **Reproduce 2.11's layouts, not 2.5's.** The engine compiles against
  `granny211.h`. blendergranny and nwn2mdk remain the sources for the *file* format,
  which is unchanged, but not for the in-memory structures.
- **Going back is not an option for x64.** Granny 2.5 has no 64-bit build at all, so
  there is nothing to fall back to.
- **The blend layer is documented after all.** Both SDKs carry `granny2.chm`, 2.5 MB
  and 4.7 MB. That covers the control and ease API that M4 needs and that no open
  source project implements. It is documentation, not source, so reading it is not a
  clean-room problem the way leaked source would be. See below.

### The documentation, and how it may be used

Both `granny2.chm` files decompile with `hh.exe -decompile <dir> granny2.chm`, which
is a standard Windows tool, into one HTML page per entry point:

| | pages | version stamped on each page |
|---|---|---|
| `vendor.rar`, the original Nival SDK | 984 | 2.5.0.5 |
| `Granny_Common_2_11_8_0_Release.zip` | 1,274 | 2.11.8.0 |

**All 54 entry points this port needs are documented in both.** The 2.11 set also
carries a `Compatibility` page and a changelog covering the 2.5 to 2.11 breaking
changes, which is RAD's own account of the migration this document reconstructed by
diffing headers and probing the DLL, and therefore a cross-check on the eight
changed structures listed above.

The content is substantive rather than a signature dump. `GrannyEaseControlIn`, for
one, gives the ease curves as Beziers with explicit start and end times, start and
end values and tangents, and states that easing a control does not set its weight,
only the curves that modify it. That is the part of M4 the plan called low
confidence for want of prior art. There is prior art: it is the vendor's manual.

**Reading it is the cleanest provenance available here, not the most doubtful.**
Documentation exists to tell an implementer what a function does; using a documented
behaviour is using a fact, and facts and interfaces are not the protected part. It is
a strictly better source than what this port has mostly relied on so far, which is
inferring behaviour by probing a binary. Where the two disagree, the binary wins,
because the binary is what the game links against.

What is not allowed, and matches the policy `blendergranny` states in its
`docs/PROVENANCE.md` and this project follows:

- **The CHM and anything extracted from it stay out of the repository.** They are
  RAD's copyrighted text and committing them would be redistribution. Keep them
  beside the DLLs, as a local reference.
- **No prose copied into comments, and no sample code copied into implementations.**
  Read it, understand it, write our own. That costs nothing and removes the question
  entirely.
- **Cite the fact, not the page.** A comment saying what a function does is a
  statement about the API; a comment quoting the manual is a copy of it.

## Validation against `granny2.dll`

The DLL can be driven directly from Python with `ctypes`, which makes it a scriptable
oracle. `granny_int32x` is 32-bit even on x64, so no thunking is needed, and the
structs are packed (see above). The oracle drives **2.11.8.0**, the version the engine
now targets, rather than the 2.5.0.5 the game shipped with.

### Decompression: bit-exact

| comparison | result |
|---|---|
| blendergranny vs DLL, repo beta | **15,742 of 15,742 identical SHA-256** |
| DLL over base + FoTR + TC | **83,184 files decompressed, 0 failures** |
| blendergranny vs DLL, base / fotr / tc | 784 / 783 / 2,590 identical, 0 mismatches (1-in-20 sample) |
| DLL vs blendergranny vs granny-ro-js, Oodle0 | **240 of 240 identical** |
| DLL vs blendergranny, Oodle1 | **160 of 160 identical** |

The DLL processes the whole beta corpus in 18 seconds; blendergranny takes about
144 ms per file, so a full pure-Python pass is roughly 40 minutes.

### Pose: close, but not faithful on all clips

The full DLL pose chain is also scriptable: `ReadEntireFileFromMemory`,
`GetFileInfo`, `InstantiateModel`, `BeginControlledAnimation`, `SetTrackGroupTarget`,
`EndControlledAnimation`, `SetModelClock`, `SampleModelAnimations`, `BuildWorldPose`,
`GetWorldPoseComposite4x4`.

Compared against granny-ro-js `poseAt()` over 18 clips and 318 bones:

- Positions agree to **3.7e-07** everywhere, essentially exact.
- 12 of 18 clips agree at float32 ULP level (about 1.2e-07); one clip is
  bit-identical throughout.
- **3 clips diverge** through quaternion double-cover sign flips.

Dense re-sampling of those three (200 samples per clip rather than 9) found the true
magnitude:

| unit | bone | clip length | peak error |
|---|---|---|---|
| M5 Satan | `Turret` | 0.633 s | **47.3 deg** at t=0.53 |
| 4.5 inch Gun (GB) | `RearWheels02` | 1.300 s | **44.8 deg** at t=0.24 |
| Pz III Ausf J | `Turret` | 0.633 s | **44.4 deg** at t=0.38 |

**Cause.** A quaternion and its negation are the same rotation, so a sign flip on a
final pose is invisible. It becomes visible during *interpolation*: if adjacent
B-spline control points sit in opposite hemispheres, the blend travels the long way
round the sphere. Granny neighbourhoods control points so consecutive ones have
positive dot product; granny-ro-js does not, on these clips. The measured error
profile confirms it, being near zero at the knots and peaking mid-segment:

```
M5_Satan / Turret, error in degrees over a 0.633 s clip
0.00=0  0.09=2  0.18=1  0.27=1  0.36=1  0.40=4  0.45=12  0.49=29  0.53=45  0.58=20  0.62=6
```

**Fix.** Walk the control points once at parse time and negate any whose dot product
with the previous one is negative.

**Confirmed by a third implementation.** noclip.website samples the same era of files
and never exhibits this, because it ignores `degree` entirely and slerps between the
two bracketing control points; `gl-matrix`'s `quat.slerp` negates its second operand
when the dot product is negative, so it takes the short path for free. That isolates
the defect to the missing neighbourhooding pass rather than anything deeper in the
spline evaluation:

| | curve model | short path | against the DLL |
|---|---|---|---|
| `granny2.dll` | degree-2 B-spline, neighbourhooded controls | yes | reference |
| granny-ro-js | degree-2 B-spline, DLL-faithful float behaviour | **missing** | 47 deg on 3 of 18 clips |
| noclip | ignores degree, piecewise slerp | free, via `gl-matrix` | never faithful, never broken |

**A viewer-grade fallback exists.** Since animation is presentation-only, noclip's
approach is a legitimate option if degree-2 B-spline evaluation proves awkward: it is
what a shipping renderer actually does. The caveat is that a B-spline does not pass
through its control points, so on the 26% of curves that are not constant this will
visibly differ from the DLL. Measure the difference with the oracle before choosing
it rather than assuming it is acceptable.

**User-visible symptom.** A turret or wheel snaps roughly 45 degrees off axis and
back over about 0.2 s, part way through a short clip. Children inherit it, so a
tank's whole turret assembly swings. If the clip loops or replays per shot the
twitch recurs, reading as rhythmic jerking. It does **not** produce NaN, vanishing
geometry, or frozen models; none were observed.

**Harness lesson.** The original 9-samples-per-clip pass reported a residual of
3.6e-02 in quaternion components, which reads as "1 to 4 degrees, minor". Dense
sampling found 47 degrees. Coarse sampling lands near knots, which is exactly where
this class of bug hides. **Sample densely, between knots.**

## Determinism: animation is presentation-only

This determines the acceptance bar for a replacement, so it was traced explicitly.

1. **`AILogic` does not link `3Dmotor`.** Its dependencies are `B2_M1_Terrain
   B2_M1_World Common_RTS_AI DebugTools Input libdb MemoryLib Misc Script
   Stats_B2_M1 System zlib fmt`. It cannot call the animation runtime.
2. **`Common_RTS_AI`**, which holds the simulation's real collision and pathfinding
   (`AIMap`, `Collision`, `CollisionInternal2`, `CommonPathFinder`, `BasePathUnit`),
   links only `DebugTools Image libdb MemoryLib Misc System zlib fmt`. Also no
   `3Dmotor`.
3. **There are two unrelated `CAIMap` classes.** `AILogic` uses
   `Common_RTS_AI/AIMap.h`. The one in `3Dmotor/aiMap.cpp` is file-local, implements
   `NAI::IAIMap`, and is the one holding animated skinned hulls via `CSkinner`. It is
   populated by `SceneB2`'s `CAIMapVisitor`, which syncs from `IVisObj`, that is from
   visual objects.
4. **Both bridge modules are clean.** `B2_M1_Terrain` has zero animation usage, its
   only `CAIMap` mention being commented out. `B2_M1_World` uses animation heavily
   but is client side, linking `SceneB2`, `Sound`, `Main` and `3Dmotor`, and
   `AILogic`'s entire surface into it is one header, `CommonB2M1AI.h`, declaring a
   single abstract interface with no geometry payload. The direction is inverted from
   what it looks like: `IAILogic : public ICommonB2M1AI`, so the client calls into
   the simulation through it.
5. **Every `GetBonePosition` call site is presentation**: sound placement
   (`MapObj.cpp:972` into `SoundScene()->AddSound`), muzzle effects and recoil in
   `AIUpdateShot` reacting to an `SAINotifyMechShot`, smoke trails, and
   `GetFirePoint` feeding `CLaserMarkTrace` shot-trace rendering
   (`UpdatableWorld.cpp:2198`).
6. The codebase marks the boundary explicitly with `AI2Vis` and `Vis2AI` conversions.

**Consequence: a replacement does not need bit-exactness with `granny2.dll`.** The
granny-ro-js divergence above is a visual quality bug, not an ASYNC risk. Comparisons
against the DLL should be tolerance based and *reported as a metric*, not pass/fail.

## Plan

### Architecture decisions

**Implement the Granny API shape first, refactor to a neutral one later.** Exposing
the same 54 entry points with the same signatures means `GAnimation.cpp`,
`GBind.cpp`, `GObjectInfo.cpp`, `aiObjectLoader.cpp`, `TerraTools.cpp` and the four
`SceneB2` mutators compile untouched. The reason is validation, not laziness: on
Windows both implementations can run side by side in one process and be asserted
call for call. That leverage disappears the moment the API changes.

Note there are two distinct "C API" surfaces with different lifetimes:

- **The data structs** (`granny_file_info`, `granny_mesh`, `granny_bone` and so on).
  The engine walks these by hand in twenty-odd places. These must be produced
  exactly, and they remain until `GObjectInfo` and friends are refactored.
- **The function surface** (opaque handles, builders, controls). Pure scaffolding,
  to be deleted at M6.

**Parse into owned native structs; do not memory-map the file.** The files store
32-bit pointers and x64 needs 8-byte ones. Since the replacement implements
`GrannyGetFileInfo` itself, it can simply allocate and populate. The 32/64 problem
disappears, x86 and x64 share one path, and the assets are tiny, the largest geometry
being 65 KB compressed.

**Resolve fields through the type tree, not hardcoded offsets.** See the four tags
above.

**GLM is already a dependency** (`cmake/glm.cmake`, used in
`3Dmotor/GSSEtransform.h`, which already has a `LoadMatrix()` handling the row-major
to column-major conversion). Use it for quaternion-to-matrix, matrix multiply,
inverse and normalise. It does **not** help with the B-spline-over-quaternions
sampler, which is the actual hard part of M3. Two traps: three matrix conventions are
in play (`SHMatrix` row-major, GLM column-major, `granny_matrix_4x4` row-vector), so
convert at exactly one boundary; and Granny stores quaternions `x,y,z,w` while
`glm::quat(w,x,y,z)` takes w first. Keep GLM out of the public headers and expose
plain float arrays.

### Repository layout

Start in this repository, structured so that extraction later is free:

```
libgr2/
  LICENSE            MIT, independent of the Nival grant
  CMakeLists.txt     builds standalone and via add_subdirectory
  src/               C++17 core: container, codecs, type tree, parse, pose, blend
  compat/            optional target: extern "C" Granny-shaped facade
  tests/             fixtures and harness, no game required
```

Top level, **not** under `Versions/Temporary/Engine/Sources/`, which is Nival's code.
Hold one rule: `libgr2/src/` includes nothing from the engine. No `System/Basic.h`,
no `CObjectBase`, no `CPtr<T>`, no `IBinSaver`. Pure C++17 and the standard library.
Then `git subtree split --prefix=libgr2` extracts it with history intact once the API
has settled.

### Milestones and estimates

Assumes one experienced C++ developer already fluent in this tree; "day" is about six
focused hours.

| stage | work | estimate | confidence |
|---|---|---|---|
| **M0** header | 54 prototypes plus about 15 packed structs and the enums the engine touches. `scripts/port/gen-granny-stub.py` already parses every signature. Validate by building the engine against it while still linking the real DLL | **1 day** | high |
| **M1a** container | sections, fixups, marshalling, parse into owned structs | **2-3 days** | high |
| **M1b** Oodle1 | `nwn2mdk/gr2_decompress.cpp` is already C++, 330 lines, Apache-2.0, verified on this data | **0.5 day** | high |
| **M1c** Oodle0 | transliterate `blendergranny/oodle0.py`, 470 lines. Varbits, adaptive arithmetic decoder, LZ dictionary | **2-4 days** | medium |
| **M2** geometry | type-tree walker and struct population; the five small query functions. **Game draws static models** | **3-5 days** | medium-high |
| **M3** pose | curve sampling (six cases above), local pose, world pose, composite skinning, quaternion neighbourhooding | **3-5 days** | medium |
| **M4** controls | about 30 entry points. No open source prior art, but all of them are documented in `granny2.chm`, ease curves included. Bounded by what `CSkeletonAnimator` actually does | **5-10 days** | medium |
| **harness** | golden record and replay, corpus sweep, live A/B shim, malformed-input fuzzing | **5-7 days** | medium-high |
| **M5** integration | CMake, delete `granny.cmake` and the DLL and the `uesp-esoapps` submodule, x86 and x64 CI green | **2-3 days** | medium |
| **tail** | visual bugs found in play | **3-5 days** | low |

**Total: 27 to 44 working days**, roughly 6 to 9 weeks full time, 200 to 350 hours.
At 10 hours a week that is 5 to 8 months; at 20, about 2.5 to 4 months.

Visible milestones: files parse and validate at about 1.5 weeks; static models render
at about 3 weeks; things animate at about 4 weeks; blending correct and the DLL
deleted at 6 to 9 weeks.

**What moves the number.** Taking nwn2mdk's container and Oodle1 wholesale is the
single biggest saving. Because animation is presentation-only, M4 can ship crude, a
linear weight ramp without faithful ease curves, turning 10 days into about 3. The
main technical risk is Oodle0 transliteration, mitigated by the per-section DLL
oracle, which bisects to the exact file, section and diverging byte. The larger risk
is M4 hiding state semantics that only appear in game, which is what golden record
and replay exists to catch. **Build the harness before M4, not after.**

Not included: the neutral-layer refactor (M6), editor-side exporter work, or a GR2
writer.

For calibration: blendergranny is about 3,000 lines of Python covering M1 to M2 plus
parsing; granny-ro-js about 8,000 lines of JS covering M1 to M3 including pose.
Expect 3,000 to 4,000 lines of C++ for M1 to M3, plus M4 which nobody has written.

## Test harness design

Three approaches, complementary rather than alternative, in priority order.

**1. Golden record and replay, build this first.** `scripts/port/gen-granny-stub.py`
already parses all 54 signatures and `GrannyStub.cpp` already records call name,
global ordinal, arguments and per-function counts. Extending it from logging to
recording inputs *and outputs* is an incremental change to code that exists. It
captures the real input distribution including call sequences and object lifetimes,
and crucially **it replays offline with no DLL and no game, so it runs in CI on
Linux**, which neither other approach can. Record output hashes for the hot functions
(`SampleModelAnimations` and `GetWorldPose4x4` run per bone per frame) and full
detail for rare ones; bound capture to a window.

**2. Offline corpus sweep.** For anything derived purely from a file, the corpus *is*
the input space; enumeration is exhaustive and cheap. Widen beyond the base game:
TC alone is 3.3x the corpus and exercises tag `0x80000011`.

**3. Live side by side.** Highest fidelity, worst ergonomics: Windows only, needs the
game running, cannot run in CI. Build it as a *debugger* for when a golden replay
fails and you need to see where divergence first appears, not as a regression suite.

**Fuzzing, narrowly.** Do not fuzz the API surface; the engine reaches a tiny
stereotyped corner of it, for example `GrannySetTrackGroupAccumulation` has one call
site and is only ever passed `GrannyNoAccumulation`. Two useful places: malformed
file robustness, where the oracle is "does not crash" rather than "matches", since
the real DLL happily returns null and lets callers dereference it; and blend-space
sampling within ranges `CSkeletonAnimator` can actually generate.

**Comparison policy.** Structural data (bone names, counts, parent indices, topology,
vertex counts, material grouping) compares **exactly**. Float data compares with a
**tolerance reported as a metric**. "12 of 18 clips at 1e-07, 3 clips at 47 degrees
on `Turret`" is far more useful than "FAIL".

**The corpus cannot be committed.** 83,184 files, 15.5 GB, and it is Nival's
copyrighted data. Commit *manifests*, mapping path to SHA-256 of decompressed
sections, and hand-author a few tiny GR2 fixtures: one per tag, one per compression
type, one malformed.

## Licensing notes

Not legal advice; this records what was observed.

**Patents are probably not the issue.** Oodle0 and Oodle1 date to roughly 1999-2004.
A US utility patent runs 20 years from filing, so anything covering them has expired.
LZ77 and arithmetic coding are long out of patent. No patent search was performed;
that would need an assignee search on RAD Game Tools and Epic Games.

**Reverse engineering for interoperability is well supported.** EU Software Directive
2009/24/EC Art. 6 permits decompilation for interoperability and Art. 5(3) permits
observation and study. In the US, *Sega v. Accolade* and *Sony v. Connectix*
established intermediate copying for interoperability as fair use.

**The concrete exposure is what is already in the tree.** `cmake/granny.cmake` does
`install(FILES "${DLL}" DESTINATION bin)`, redistributing RAD's `granny2.dll`
(version 2.11.8.0, "Copyright 1999-2017 RAD Game Tools"), sourced from
`third_party/uesp-esoapps`, whose copy is licensed to ZeniMax for Elder Scrolls
Online. The binary itself is byte-identical to the general 2.11.8.0 SDK release, so
nothing about it is specific to that game, but the copy being redistributed is theirs.
Note that this ships a *newer* Granny than the one the game was licensed with:
Blitzkrieg 2 shipped 2.5.0.5. `granny2_static.lib` at 6.8 MB, `gstate.lib` and
`granny211.h` are in the same submodule. Writing a minimal header with the 54 prototypes is both the
interoperability-necessary subset and a way to delete RAD's 6,000-line SDK header on
day one.

Do not name a replacement "Granny"; that is RAD's trademark. The compatibility facade
may declare Granny-named *functions* for source compatibility.

Note also that `LICENSE.md` is a Nival non-commercial grant that the licensor may
revoke at will with three days' notice. That is the main argument for keeping
`libgr2/` under its own MIT licence and eventually in its own repository, and it
matters most for attracting outside help on Oodle0 and the blend layer.

## Reproducing the measurements

The DLL oracle used throughout is committed as
[`scripts/port/granny_dll_oracle.py`](../scripts/port/granny_dll_oracle.py). It
exposes decompression and the full pose chain, and needs a 64-bit Python.

```powershell
# decompress every section of a file through the real DLL, print SHA-256
python scripts/port/granny_dll_oracle.py hash <file.gr2> ...

# dump local transforms, world matrices and composite skinning matrices as JSON
python scripts/port/granny_dll_oracle.py pose <file.gr2> <t0> <t1> ...
```

The version comparison needs no tooling beyond what ships with Visual Studio.
`Get-FileHash` collapses the five DLL copies to two; `(Get-Item $dll).VersionInfo`
gives the version, and bytes 8 to 12 past the PE header offset at `0x3C` give the link
date. Export sets come from `dumpbin /exports`, matching `_(Granny\w+)@(\d+)`: on x86
the `@N` is the argument size in bytes, so comparing those across two DLLs finds every
changed signature without reading a header. Structure layouts were diffed between
`vendor.rar`'s `granny.h` and `granny211.h`, matching `^struct granny_(\w+)` in the
first and `GRANNY_STRUCT\(struct\) granny_(\w+)` in the second.

Census scripts were written ad hoc; the shapes worth keeping are:

- `.pak` files open with Python `zipfile`; filter entries on
  `bin/(geometries|animations|skeletons|aigeometries)/`, de-duplicate on
  `(basename, CRC, size)`.
- Header census needs only the first 512 bytes per entry, so `z.open(info).read(512)`
  stops deflate early.
- Curve census uses `blendergranny`: `read_gr2_bytes`, then `load_sections`, then
  `extract_animation_set`, then `CurveMetadata.codec` / `.degree` / `.dimension`.

External references, all read-only clones:

| project | path | licence |
|---|---|---|
| opengr2 | `/c/projects/opengr2` | MPL-2.0 |
| MacPak / MacLarian | `/c/projects/MacPak` | PolyForm-NC |
| nwn2mdk | `/c/projects/nwn2mdk` | Apache-2.0 |
| Knit | `/c/projects/Knit` | EUPL-1.2 |
| blendergranny | `/c/projects/blendergranny` | MIT |
| granny-ro-js | `/c/projects/granny-ro-js` | MIT |
| noclip.website | `/c/projects/noclip.website` | MIT, not clean-room |
