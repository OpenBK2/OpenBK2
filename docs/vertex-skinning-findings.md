# Vertex skinning: reachability, a latent import bug, and the MMXTransformVector divergence

Findings from 2026-08-23. Nothing here has been changed in the engine; this is the
record so the work does not have to be redone.

## 1. A latent bug in the Granny weight import

`3Dmotor/GObjectInfo.cpp`, in the branch that reads per-vertex bone weights out of a
Granny mesh:

```cpp
granny_uint8 weights[4];                                    // uninitialised
memcpy( weights, pVertex + nWeightsOffset, nWeightsCount * sizeof(granny_uint8) );
wData.fWeights[0] = weights[0] / 255.0f;
wData.fWeights[1] = nWeightsCount > 0 ? weights[1] / 255.0f : 0.0f;   // should be > 1
wData.fWeights[2] = nWeightsCount > 1 ? weights[2] / 255.0f : 0.0f;   // should be > 2
wData.fWeights[3] = nWeightsCount > 2 ? weights[3] / 255.0f : 0.0f;   // should be > 3
```

Every guard is off by one. With `nWeightsCount == 1` only one byte is copied into
`weights`, yet `fWeights[1]` is still read from `weights[1]`, which is uninitialised
stack. `CObjectInfo::AssignGeometry` then sorts descending and quantises with
`Clamp( Float2Int( f * 256 ), 0, 255 )`, so that garbage can surface as a non-zero
`nWeights[1]` and send the vertex down the two-bone path with a fabricated second
bone. `cBoneIndices` has the same problem via `nIndicesCount`.

**This is Nival's, not the port's.** It is present verbatim in commit `3726f2d9d`
("Add engine sources"), and every later commit touching the file was cosmetic
(include casing, `const char *`, `std::min`/`std::max`, trailing newlines).

**It is inert on all data checked.** Every skinned mesh in the base game, the FoTR
addon and the Total Conversion declares `BoneWeights[4]`, so `nWeightsCount == 4`,
all three guards are satisfied for the right reason, and all four bytes really were
copied. It would only bite a mesh declaring 1 to 3 weight slots, i.e. a modder's
export with a narrower vertex format, and it would do so nondeterministically.

Fixing it is a one-line change per guard plus zero-initialising `weights` and
`indices`. Left undone deliberately: it deserves its own change with its own
reasoning, not a drive-by.

## 2. The 2- and 3-bone paths are live

`3Dmotor/GCombiner.cpp`, `SingleSkinTransform`, picks per vertex:

```cpp
if ( pWeight->nWeights[1] == 0 )        MMXTransformVector( ... );   // 1 bone
else if ( pWeight->nWeights[2] == 0 )   MMXTransformVector2( ... );  // 2 bones
else                                    MMXTransformVector3( ... );  // 3 bones
```

Measured by decompressing every `.gr2` in the shipped `.pak` archives with the Granny
SDK and querying the vertex format exactly as `GObjectInfo.cpp` does:

| dataset | meshes | with `BoneWeights[4]` | 1 bone | 2 bones | 3 bones | 4 bones |
|---|---|---|---|---|---|---|
| base, `bin/Geometries` (render) | 4797 | 159 | 98.29% | 0.30% | 1.41% | 0% |
| base, `bin/AIGeometries` | 1234 | 9 | 0% | 9.09% | 90.91% | 0% |
| FoTR addon | 6958 | 171 | 99.13% | 0.14% | 0.72% | 0% |
| Total Conversion mod | 62905 | 539 | 43.22% | 13.97% | 21.75% | 21.05% |

Percentages are of vertices within meshes that carry weights.

So both paths execute on original game data, and they are heavily used by the Total
Conversion. An earlier guess that they were dead code, based on the rigid import path
forcing `{1,0,0,0}`, was wrong.

### What actually uses them in the base game

Only 15 render models contain any multi-bone vertex. Resolving the model GUID back to
its `<uid>` in the database gives:

- `Units/Infantry/Animals/Dog/Dog/summer_dog_geometry.xdb`
- `Units/Infantry/Animals/Pig/Pig/summer_pig_geometry.xdb`
- `Objects/SimpleObjects/Cow/2/summer_1_geometry.xdb`
- `Objects/SimpleObjects/Cow02/1/summer_1_geometry.xdb`
- `Objects/SimpleObjects/Sheep/1/summer_1_geometry.xdb`
- `Objects/SimpleObjects/Dog/1/summer_1_geometry.xdb`
- `Objects/SimpleObjects/Washing/1/summer_1_geometry.xdb`
- `Buildings/Common/bigpig/summer_1_geometry.xdb`

The remaining 7 GUIDs appear in no `.xdb` at all and look like orphaned content.

No vehicle and no soldier. That fits the engine: vehicles and infantry are rigidly
bound, one bone per part, and smooth skinning only shows up where something deforms
organically. A live test wants a map with cows, sheep, pigs, dogs and a washing line,
not an armour showcase.

### The engine drops a fourth influence

`SingleSkinTransform`'s final `else` covers "3 or more" and calls
`MMXTransformVector3`, which takes three weights. `nWeights[3]` is never used, so a
4-bone vertex silently loses its fourth influence. That is 21% of the Total
Conversion's skinned vertices. Note the asymmetry: the *position* path
(`TransformPosition` / `SSESkinning`) does handle four bones, so on those meshes
positions blend four influences while normals blend three.

## 3. How large is the glm divergence, really

`MMXTransformVector*` in `GSSEtransform.h` is a float rewrite; the original was
fixed-point MMX. Note which float rewrite: `HAS_SSE2` is 1 on both x86 and x64 under
default flags (`_M_IX86_FP == 2`), so the single-transform case actually compiles the
SSE2 intrinsic version, and the glm one in the `#else` branch is dead code on every
supported configuration. `MMXTransformVector2` and `3` have no SSE2 variant, so those
two are glm. Either way it cannot be made bit-exact, because `Assign()` quantises the matrix
to 16-bit fixed point (`Float2Int( value * 2048 )`) before any multiply and the
pipeline then runs in saturating 16-bit with truncating shifts and a normalize lookup
table. Transcribing the same pipeline with the `mmx::` integer helpers is exact; a
float implementation cannot be.

Measured over 200000 random cases (`3Dmotor/test/original/divergence`):

| | vectors exact | worst component error |
|---|---|---|
| 1 bone (SSE2 intrinsics) | 16.76% | 1 |
| 2 bones (glm) | 11.31% | 77 |
| 3 bones (glm) | 9.96% | 76 |

The worst-case numbers look alarming and are misleading. Conditioning the two-bone
error on the angle between the two blended transforms:

| angle between transforms | samples | vectors exact | worst component error |
|---|---|---|---|
| 0-20 deg | 3923 | 12.82% | **1** |
| 20-40 | 11251 | 12.10% | **1** |
| 40-60 | 18162 | 12.62% | **1** |
| 60-80 | 23171 | 12.28% | **1** |
| 80-100 | 26853 | 11.87% | **1** |
| 100-120 | 29501 | 11.12% | **2** |
| 120-140 | 30283 | 10.79% | **2** |
| 140-160 | 28856 | 10.45% | **2** |
| 160-180 | 28000 | 10.08% | **86** |

Every large error lives in the last bucket. Blending two nearly opposite rotations at
roughly equal weight produces a near-zero vector, and normalising that is unstable, so
the table-driven fixed-point normalize and the float one diverge hard. That regime is
an artifact of pairing independent random rotations; adjacent bones in a real skeleton
are nowhere near 160 degrees apart.

Bucketing the same comparison by the length of the blended normal *before*
normalization says the same thing more directly:

| blended length | samples | vectors exact | worst component error |
|---|---|---|---|
| 0.01 - 0.05 | 9 | 0.00% | 47 |
| 0.05 - 0.10 | 74 | 1.35% | 24 |
| 0.10 - 0.25 | 1282 | 2.96% | 4 |
| 0.25 - 0.50 | 10500 | 6.61% | 2 |
| > 0.50 | 188135 | 11.78% | 2 |

The error is inversely proportional to how much normal survives the blend. Above 0.25,
which is 99.3% of cases, the worst error is 2. The large errors need the blend to have
almost cancelled, 83 samples in 200000.

That matters for what "fixing" this would buy. A blended normal of length 0.05 has no
meaningful direction left; its direction is numerical noise. In that regime the
original MMX is not the correct answer that glm misses, it is an equally arbitrary
answer. Making the two bit-identical there would not remove a rendering artifact, it
would only make our arbitrary normal match Nival's arbitrary normal.

**So the practical answer is that the divergence is 1, occasionally 2, out of 255 on a
normal component.** That is the same order as the single-bone case, and consistent
with never having been noticed in rendering. Anything visible would require a model
whose blended bones are close to opposed, which is a broken rig rather than a
rendering bug.

## 3a. What a wrong normal can and cannot do

`MMXTransformVector*` transforms **normals**. In `GCombiner.cpp` the results go into
`xformedNormals`, whose only consumer is `CalcPerVertexLight`; that uses them for
`CalcDirectionalLighting` and the point light kernels, and copies them into
`res.normal` on the output vertex. Positions arrive separately, already transformed by
`TransformPosition` / `SSESkinning`, which this divergence does not touch at all.

So the only possible symptom is **shading**: a vertex lit as though it faced a
different direction, appearing as a soft bright or dark patch that can flicker as an
animation passes through the degenerate pose. Per-vertex lighting is interpolated
across the triangle, so it would be a blotch rather than a hard edge.

It cannot cause geometry symptoms. Shaking, teleporting, detached or sunken parts,
impossible joint angles, vertices flying off - none of those can come from this code,
because it never touches a position. If any of those are ever observed, look at
`TransformPosition` / `SSESkinning` and the animation code instead, and note that
`SingleSkinTransform` dropping the fourth bone influence (see above) is a far more
plausible cause of visible skinning artifacts on Total Conversion models.

## 4. Reproducing the data scan

The models are Granny `.gr2` but are stored **with no file extension** under `bin/`
inside the `.pak` archives, which are ordinary zips. Detect them by the Granny magic
`b8 67 b0 ca f8 6d b1 0f`.

Do not try to grep the raw bytes for `BoneWeights`: the type-name strings are
compressed and do not appear, and neither do `Position` or `Mesh`, so a zero result
means nothing. The vertex format has to be read through the Granny SDK. Link
`third_party/uesp-esoapps/common/granny/win64/granny2_x64.lib` and include both
`Versions/Temporary/Engine/Sources/vendor/granny/include` (a shim) and
`third_party/uesp-esoapps/common/granny` (the real `granny211.h`), then query
`CalcGrannyTypedefOffset` / `CalcGrannyMemberArraySize` against
`GrannyVertexBoneWeightsName` the same way `GObjectInfo.cpp` does.

The `.xdb` database files are **UTF-8**, not UTF-16, and a geometry record names its
model in a `<uid>` element whose value is the `.gr2` filename.

## 5. Finishing the x64 test coverage, and why not with MASM

Six tests still compare against the inline `__asm` in `test/original.h` and so are
registered on x86 only: `MultiplyOnColor`, `CalcPointLightColors` (both overloads),
`AddColors`, `ScaleColors`, `SampleWarFogInt` and `SampleWarFog`. Between them that is
seven functions, 2 to 4 `__asm` blocks each and at most 39 instructions:

| function | asm blocks | instructions | globals it names | callee-saved registers |
|---|---|---|---|---|
| `MultiplyOnColor` | 2 | ~10 | - | esi |
| `SampleWarFogInt` | 1 | small | - | - |
| `SampleWarFog` | 1 | ~5 | - | edi, esi |
| `CalcPointLightColors` (indexed) | 3 | ~34 | - | edi, esi |
| `CalcPointLightColors` (uniform) | 4 | ~39 | - | edi, esi |
| `AddColors` | 3 | ~38 | `nCubicRoot` | ebx, esi |
| `ScaleColors` | 4 | ~17 | - | esi |

**Prefer SSE2 intrinsics over MASM.** MASM fixes the architecture and nothing else: it
is `ml`/`ml64` only, so a Linux or macOS port would need every one of these written a
second time in NASM or GAS syntax. Every operation these functions use is from the set
already shown to translate exactly - see `test/original/MMXPrimitives.h`, which
replaced thirty inline wrappers with intrinsics and passes on both architectures. SSE2
defines these integer operations as MMX does, and they are all either lane-wise or
shift within a 64-bit lane, so the low half of an `__m128i` reproduces the 64-bit MMX
result.

Two wrinkles to expect:

- The multi-block functions load loop-invariant values into MMX registers before the
  loop and use them inside it (`CalcPointLightColors` does this with `mm7` and `mm5`).
  A separate function cannot inherit register state, so those get reloaded per call.
  That costs speed and changes no results, which is the right trade for a reference.
- `punpckhwd` and the two packs need the handling documented in `MMXPrimitives.h`.

Verify the same way each time: transliterate, then have `test/original/sentinel` compare
the new form against the inline `__asm` on x86 before trusting it on x64.

The MASM already written for `CalcDirectionalLighting` and `MMXTransformVector` works
and is proven bit-identical over 200000 cases each, but carries the same portability
cost. It could be converted on the same argument, keeping the assembly alongside as the
thing the conversion is checked against and dropping it once both architectures are
green. `MMXTransformVector` is the weakest candidate: its `nNormalizeTable` lookup and
`ebx` handling are captured literally by the assembly, where intrinsics would express
them as C++ around the SIMD - still verifiable, but a transliteration rather than a
transcription.
