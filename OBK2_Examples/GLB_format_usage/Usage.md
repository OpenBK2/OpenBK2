# GLB format usage

In `ExampleDinosaurUnit` folder, you'll find the animated dinosuar model and its configuration for the game.

The engine and editor accept `.glb` and `.gltf` models. Rendering still uses the game's XDB materials and textures; embedded glTF materials do not replace them.

Geometry and AIGeometry select mesh nodes through `RootMesh`; Skeleton uses `RootJoint`. You can store both the collision mesh and the visible model in one file: for example, use `Basis` for Geometry.RootMesh and Skeleton.RootJoint, and `AABB` for AIGeometry.RootMesh.

When doing animation clip splitting, either have all animations in one big clip in GLB (that's when you use FirstFrame and LastFrame properties), or have all of them in their own separate clips (aka actions in Blender) - that's when you use ClipName property. Setting both at once is not supported: a non-empty ClipName selects the whole clip and the frame range is ignored. That combination is reported in the log, along with the two other ways frame slicing can fail, a GLB exported without animation sampling and a range outside the timeline the exporter baked. This is still an improvement from the old maya where all clips had to be in one sequence and then exported to separate GR2 files. 

Locators should probably be rotated 90 degrees along the X axis to show effects properly.

## Creating and exporting a VisObj

In **Create New <VisObj> Object**, select a `.glb` or `.gltf` in `ModelFileName`. Keep using `TextureFileName`, `TextureType`, `RootMesh`, `RootJoint`, `AIRootMesh`, and the optional shared `Skeleton` as before. The creator fills `ModelFileRef` on Geometry, AIGeometry, and newly created Skeleton resources; it leaves their legacy `SrcName` fields empty.

Models selected outside the data/mod folder are copied beside the new object's XDB files, even when **Export object after creation** is unchecked. A `.gltf` package includes its referenced buffers and images, with their relative paths preserved. The `ModelFileRef` browse button also imports external models beside an existing resource. Stored model references are relative to the data/mod folder.

Seasonal suffixes remain `w`, `s`, no suffix, `u`, `a`, and `i`. Missing seasonal files fall back to the base model or texture. The batch creator retains the numbered `1`, `2`, and `3` model convention using the selected `.glb` or `.gltf` extension.

Exporting VisObj, Model, Geometry, AIGeometry, Skeleton, or AnimB2 uses the GLTF path. Export refreshes geometry bounds and animation lengths; named clips matching the existing animation types (such as `idle` or `walk`) can populate an empty Skeleton animation list. Other clips can be assigned manually using AnimB2's `ClipName` and `Type` fields. Existing GR2 resources remain loadable, but Maya/GR2 source export is no longer supported.

## Object and building export

`ObjectRPGStats` and `BuildingRPGStats` export from GLB/GLTF only. Set the visual object's Geometry and Skeleton `ModelFileRef` fields, then export the stats resource with references. Existing GR2 assets remain loadable. Object height, surface points, debris masks and passability are generated from the selected GLTF mesh in engine coordinates; these exporters no longer wait for GR2 binaries or invoke Maya.

Buildings retain their locator and numbered-texture conventions:

- `LDoor01`, `LWindow01`, etc. supply entrance/window transforms. Optional numeric node `extras` such as `windowscalex` and `windowscaley` retain their existing meanings.
- `Lsection01` identifies the section rooted at `section01` (the exact marker name with its initial `L` removed). Each section contains stage locators ending in `Lstage01`, `Lstage02`, etc. Give them numeric `extras` named `starttime` and `endtime`, corresponding to frames in a sampled GLTF animation timeline. Number stages consecutively from 01 and use unique full node names, for example `section02_Lstage01` in a second section.
- Section export creates separate VisObj, Model, Geometry, Skeleton and AnimB2 XDB files. The generated animations use `ModelFileRef` and frame ranges, with `ClipName` empty.
- Numbered textures are read beside the original model's XDB material texture source (`Texture.SrcName`): `1.tga` for the whole section, `2.tga` for an assigned destroyed section, and `3.tga`, `4.tga`, etc. for damage stages. Mesh-node boolean/numeric `extras.transparent` selects `1t.tga`, `3t.tga`, etc.; `extras.reflective` adds `1_m.tga`, `3_m.tga`, etc. This remains independent of embedded GLTF materials and works when the model file was imported from elsewhere.

Invalid model selectors, stage metadata, missing numbered textures and debris write failures abort export and report the error. Debris source TGAs still go to the configured export source folder, so that folder must be writable.
