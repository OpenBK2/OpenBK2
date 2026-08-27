# Inspect Granny3D animation usage

> **Status.** Written 2026-08-16 and re-verified against `port` on 2026-08-27. The
> findings held; what changed is corrections to three file citations, and the
> Granny-removal section, which predated the Linux stub that pinned the link
> surface at 54 entry points. Sections added since the original: the
> `SGrannySkeletonPose` leak, the two GR2 readers outside `CObjectInfo`, and the
> save-format coupling.
>
> **Sequel.** [GrannyReplacement.md](GrannyReplacement.md) covers the other half of
> the question: six open source GR2 libraries measured against this game's data, the
> corpus censuses, bit-exact validation against `granny2.dll`, and a staged plan for
> a native reader. It supersedes this document wherever the two disagree, since it
> was measured on the retail game rather than the repository's beta snapshot.

> This game uses Granny3D as a 3D model format (granny 2 - .gr2) & management library. I want you to inspect the code around granny3D functions it uses (most, if not all of it is probably in [3Dmotor](../Versions/Temporary/Engine/Sources/3Dmotor/) project) and tell me, is it using granny as just the 3D format utility or does it also use its animation system and such (or does the game have its own animation system)?
> I'm asking because I want to know how feasable would it be to add a new 3D format support to the game (probably GLTF), and keeping granny only as a legacy, backwards compatibility thing. Don't change any code, just inspect well. Don't fret to give me a nice chunky description of how the thing works.

<details><summary>10 previous messages</summary>

> I’ll trace the Granny-facing API calls, then follow model loading, skeleton/animation ownership, runtime pose updates, and rendering paths. I’ll keep this read-only and won’t build anything, per the repo instructions.
>
> <details><summary>Ran 8 commands</summary>
>
> - Ran `rg -n -i --glob '*.{h,hpp,c,cc,cpp,cxx,inl,cmake,txt}' "granny|gr2|granny_" Versions/Temporary/Engine/Sources/3Dmotor Versions/Temporary/Engine/Sources`
> - Ran `rg -l -i --glob '!vendor/**' --glob '*.{h,hpp,cpp,inl}' "granny|\.gr2" Versions/Temporary/Engine/Sources/3Dmotor | Sort-Object`
> - Ran `rg -l -i --glob '!vendor/**' --glob '*.{h,hpp,cpp,inl}' "granny|\.gr2" Versions/Temporary/Engine/Sources/3Dmotor | Sort-Object`
> - Ran `rg -l -i --glob !vendor/** --glob *.{h,hpp,cpp,inl} "granny|\.gr2" Versions/Temporary/Engine/Sources/3Dmotor`
> - Ran `Get-Location`
> - Ran `Get-Location`
> - Ran `rg -l -i --glob '!vendor/**' --glob '*.{h,hpp,cpp,inl}' "granny|\.gr2" Versions/Temporary/Engine/Sources/3Dmotor | Sort-Object`
> - Ran `rg -o --glob '!vendor/**' --glob '*.{h,hpp,cpp,inl}' "Granny[A-Za-z0-9_]+" Versions/Temporary/Engine/Sources/3Dmotor | ForEach-Object { ($_ -split ':')[-1] } | Group-Object | Sort-Object Count -Descending | Select-Object Count,Name`
>
> </details>
>
> The first pass already answers the headline: this is not a “GR2 as mesh container only” integration. The runtime directly instantiates Granny models, creates Granny animation controls, blends/eases them, samples local poses, and builds world poses. I’m now mapping where the engine wraps that machinery and where its own animation/gameplay layer begins.
>
> <details><summary>Ran 78 commands</summary>
>
> - Ran `Get-Item Versions/Temporary/Engine/Sources/3Dmotor/GObjectInfo.h,Versions/Temporary/Engine/Sources/3Dmotor/GObjectInfo.cpp,Versions/Temporary/Engine/Sources/3Dmotor/GAnimFormat.h,Versions/Temporary/Engine/Sources/3Dmotor/GAnimFormat.cpp,Versions/Temporary/Engine/Sources/3Dmotor/GAnimUtils.h,Versions/Temporary/Engine/Sources/3Dmotor/GAnimUtils.cpp,Versions/Temporary/Engine/Sources/3Dmotor/GSkeleton.h,Versions/Temporary/Engine/Sources/3Dmotor/GSkeleton.cpp,Versions/Temporary/Engine/Sources/3Dmotor/GBind.h,Versions/Temporary/Engine/Sources/3Dmotor/GBind.cpp,Versions/Temporary/Engine/Sources/3Dmotor/GAnimation.h,Versions/Temporary/Engine/Sources/3Dmotor/GAnimation.cpp | Select-Object Name,Length,@{Name='Lines';Expression={(Get-Content $_.FullName).Count}}`
> - Ran `$i=1; Get-Content 'Versions/Temporary/Engine/Sources/3Dmotor/GAnimFormat.h' | ForEach-Object { '{0,5}: {1}' -f $i++, $_ }`
> - Ran `$i=1; Get-Content 'Versions/Temporary/Engine/Sources/3Dmotor/GAnimUtils.h' | ForEach-Object { '{0,5}: {1}' -f $i++, $_ }`
> - Ran `$i=1; Get-Content 'Versions/Temporary/Engine/Sources/3Dmotor/GObjectInfo.h' | ForEach-Object { '{0,5}: {1}' -f $i++, $_ }`
> - Ran `$i=1; Get-Content 'Versions/Temporary/Engine/Sources/3Dmotor/GSkeleton.h' | ForEach-Object { '{0,5}: {1}' -f $i++, $_ }`
> - Ran `$i=1; Get-Content 'Versions/Temporary/Engine/Sources/3Dmotor/GBind.h' | ForEach-Object { '{0,5}: {1}' -f $i++, $_ }`
> - Ran `$i=1; Get-Content 'Versions/Temporary/Engine/Sources/3Dmotor/GAnimation.h' | ForEach-Object { '{0,5}: {1}' -f $i++, $_ }`
> - Ran `$i=1; Get-Content 'Versions/Temporary/Engine/Sources/3Dmotor/GAnimation.hpp' | ForEach-Object { '{0,5}: {1}' -f $i++, $_ }`
> - Ran `$lines=Get-Content 'Versions/Temporary/Engine/Sources/3Dmotor/GObjectInfo.cpp'; for($n=1;$n -le 240;$n++){ '{0,5}: {1}' -f $n,$lines[$n-1] }`
> - Ran `$lines=Get-Content 'Versions/Temporary/Engine/Sources/3Dmotor/GObjectInfo.cpp'; for($n=241;$n -le 544;$n++){ '{0,5}: {1}' -f $n,$lines[$n-1] }`
> - Ran `rg -n --glob '*.{h,hpp,cpp}' "(struct|class) CObjectInfo|SVertexWeight|SSkeletonMatrices|CBind\b|CreateSkeletonAnimator|pAnimation" Versions/Temporary/Engine/Sources/3Dmotor | Select-Object -First 500`
> - Ran `rg -n --glob '*.{h,hpp,cpp}' "class .*CObjectInfo|struct SVertexWeight|typedef.*SVertexWeight|CObjectInfo\s*[:{]" Versions/Temporary/Engine/Sources/3Dmotor Versions/Temporary/Engine/Sources/3DLib`
> - Ran `$lines=Get-Content 'Versions/Temporary/Engine/Sources/3Dmotor/GAnimation.cpp'; for($n=221;$n -le 430;$n++){ '{0,5}: {1}' -f $n,$lines[$n-1] }`
> - Ran `$lines=Get-Content 'Versions/Temporary/Engine/Sources/3Dmotor/GAnimation.cpp'; for($n=1;$n -le 220;$n++){ '{0,5}: {1}' -f $n,$lines[$n-1] }`
> - Ran `$lines=Get-Content 'Versions/Temporary/Engine/Sources/3Dmotor/GAnimation.cpp'; for($n=431;$n -le 760;$n++){ '{0,5}: {1}' -f $n,$lines[$n-1] }`
> - Ran `$lines=Get-Content 'Versions/Temporary/Engine/Sources/3Dmotor/GAnimation.cpp'; for($n=761;$n -le 1042;$n++){ '{0,5}: {1}' -f $n,$lines[$n-1] }`
> - Ran `$i=1; Get-Content 'Versions/Temporary/Engine/Sources/3Dmotor/GAnimFormat.cpp' | ForEach-Object { '{0,5}: {1}' -f $i++, $_ }`
> - Ran `$i=1; Get-Content 'Versions/Temporary/Engine/Sources/3Dmotor/GBind.cpp' | ForEach-Object { '{0,5}: {1}' -f $i++, $_ }`
> - Ran `$i=1; Get-Content 'Versions/Temporary/Engine/Sources/3Dmotor/GSkeleton.cpp' | ForEach-Object { '{0,5}: {1}' -f $i++, $_ }`
> - Ran `$i=1; Get-Content 'Versions/Temporary/Engine/Sources/3Dmotor/AnimAnnotation.cpp' | ForEach-Object { '{0,5}: {1}' -f $i++, $_ }`
> - Ran `$i=1; Get-Content 'Versions/Temporary/Engine/Sources/3Dmotor/GAnimUtils.cpp' | ForEach-Object { '{0,5}: {1}' -f $i++, $_ }`
> - Ran `rg -n --glob '*.{h,hpp,cpp,xdb,xml}' "struct (SGeometry|SSkeleton|SAnimBase|SModel)\b|class (SGeometry|SSkeleton|SAnimBase|SModel)\b|Animations|Skeletons|Geometries" Versions/Temporary/Engine/Sources/3Dmotor/DBScene.h Versions/Temporary/Engine/Sources | Select-Object -First 500`
> - Ran `rg -n --glob '*.h' "struct SAnimBase\b" Versions/Temporary/Engine/Sources`
> - Ran `rg -n "SAnimBase" Versions/Temporary/Engine/Sources --glob '*.{h,hpp,cpp}' | Select-Object -First 300`
> - Ran `$lines=Get-Content 'Versions/Temporary/Engine/Sources/3Dmotor/DBScene.h'; for($n=350;$n -le 420;$n++){ '{0,5}: {1}' -f $n,$lines[$n-1] }`
> - Ran `$lines=Get-Content 'Versions/Temporary/Engine/Sources/3Dmotor/DBScene.h'; for($n=540;$n -le 570;$n++){ '{0,5}: {1}' -f $n,$lines[$n-1] }`
> - Ran `$lines=Get-Content 'Versions/Temporary/Engine/Sources/3Dmotor/DBScene.h'; for($n=20;$n -le 60;$n++){ '{0,5}: {1}' -f $n,$lines[$n-1] }`
> - Ran `$lines=Get-Content 'Versions/Temporary/Engine/Sources/3Dmotor/DBScene.h'; for($n=710;$n -le 770;$n++){ '{0,5}: {1}' -f $n,$lines[$n-1] }`
> - Ran `$i=1; Get-Content 'Versions/Temporary/Engine/Sources/3Dmotor/include_AnimBase.h' | ForEach-Object { '{0,5}: {1}' -f $i++, $_ }`
> - Ran `$i=1; Get-Content 'Versions/Temporary/Engine/Sources/Stats_B2_M1/DBAnimB2.h' | ForEach-Object { '{0,5}: {1}' -f $i++, $_ }`
> - Ran `rg -n -i --glob '*.{cmake,txt,h,cpp,vcproj,vcxproj}' "granny2|granny211|granny\.lib|granny.*dll|vendor/granny" . | Select-Object -First 300`
> - Ran `rg -l --glob '!vendor/**' --glob '!ED_*/**' --glob '!Arch/**' --glob '*.{h,hpp,cpp,inl}' "\b(granny_[A-Za-z0-9_]+|Granny[A-Za-z0-9_]+)\b" Versions/Temporary/Engine/Sources | Sort-Object`
> - Ran `$i=1; Get-Content 'Versions/Temporary/Engine/Sources/SceneB2/MechUnitJoggingMutator.h' | ForEach-Object { '{0,5}: {1}' -f $i++, $_ }`
> - Ran `$i=1; Get-Content 'Versions/Temporary/Engine/Sources/SceneB2/MechUnitJoggingMutator.cpp' | ForEach-Object { '{0,5}: {1}' -f $i++, $_ }`
> - Ran `$i=1; Get-Content 'Versions/Temporary/Engine/Sources/SceneB2/TreeFallingMutator.cpp' | ForEach-Object { '{0,5}: {1}' -f $i++, $_ }`
> - Ran `$i=1; Get-Content 'Versions/Temporary/Engine/Sources/SceneB2/TreeWindMutator.cpp' | ForEach-Object { '{0,5}: {1}' -f $i++, $_ }`
> - Ran `$i=1; Get-Content 'Versions/Temporary/Engine/Sources/SceneB2/TreeFallingMutator.h' | ForEach-Object { '{0,5}: {1}' -f $i++, $_ }`
> - Ran `$i=1; Get-Content 'Versions/Temporary/Engine/Sources/SceneB2/WingScaleMutator.cpp' | ForEach-Object { '{0,5}: {1}' -f $i++, $_ }`
> - Ran `$i=1; Get-Content 'Versions/Temporary/Engine/Sources/SceneB2/TreeWindMutator.h' | ForEach-Object { '{0,5}: {1}' -f $i++, $_ }`
> - Ran `$i=1; Get-Content 'Versions/Temporary/Engine/Sources/SceneB2/WingScaleMutator.h' | ForEach-Object { '{0,5}: {1}' -f $i++, $_ }`
> - Ran `$i=1; Get-Content 'Versions/Temporary/Engine/Sources/SceneB2/TerraTools.h' | ForEach-Object { '{0,5}: {1}' -f $i++, $_ }`
> - Ran `$i=1; Get-Content 'Versions/Temporary/Engine/Sources/SceneB2/TerraTools.cpp' | ForEach-Object { '{0,5}: {1}' -f $i++, $_ }`
> - Ran `rg -n "\b(granny_[A-Za-z0-9_]+|Granny[A-Za-z0-9_]+)\b" Versions/Temporary/Engine/Sources/SceneB2 --glob '*.{h,cpp}'`
> - Ran `$lines=Get-Content 'Versions/Temporary/Engine/Sources/3DLib/GGeometry.h'; for($n=1;$n -le 145;$n++){ '{0,5}: {1}' -f $n,$lines[$n-1] }`
> - Ran `$lines=Get-Content 'Versions/Temporary/Engine/Sources/3Dmotor/GParts.h'; for($n=70;$n -le [Math]::Min(220,$lines.Count);$n++){ '{0,5}: {1}' -f $n,$lines[$n-1] }`
> - Ran `$lines=Get-Content 'Versions/Temporary/Engine/Sources/3Dmotor/GCombiner.h'; for($n=1;$n -le [Math]::Min(225,$lines.Count);$n++){ '{0,5}: {1}' -f $n,$lines[$n-1] }`
> - Ran `$lines=Get-Content 'Versions/Temporary/Engine/Sources/3Dmotor/GCombiner.cpp'; for($n=1;$n -le [Math]::Min(230,$lines.Count);$n++){ '{0,5}: {1}' -f $n,$lines[$n-1] }`
> - Ran `$lines=Get-Content 'Versions/Temporary/Engine/Sources/3Dmotor/GParts.cpp'; for($n=90;$n -le [Math]::Min(220,$lines.Count);$n++){ '{0,5}: {1}' -f $n,$lines[$n-1] }`
> - Ran `$lines=Get-Content 'Versions/Temporary/Engine/Sources/3Dmotor/GCombiner.cpp'; for($n=205;$n -le [Math]::Min(460,$lines.Count);$n++){ '{0,5}: {1}' -f $n,$lines[$n-1] }`
> - Ran `$lines=Get-Content 'Versions/Temporary/Engine/Sources/3Dmotor/GCombiner.cpp'; for($n=461;$n -le [Math]::Min(760,$lines.Count);$n++){ '{0,5}: {1}' -f $n,$lines[$n-1] }`
> - Ran `$lines=Get-Content 'Versions/Temporary/Engine/Sources/3Dmotor/GView.cpp'; for($n=220;$n -le 300;$n++){ '{0,5}: {1}' -f $n,$lines[$n-1] }`
> - Ran `$lines=Get-Content 'Versions/Temporary/Engine/Sources/3Dmotor/GView.cpp'; for($n=820;$n -le 920;$n++){ '{0,5}: {1}' -f $n,$lines[$n-1] }`
> - Ran `$lines=Get-Content 'Versions/Temporary/Engine/Sources/3Dmotor/GView.cpp'; for($n=540;$n -le 640;$n++){ '{0,5}: {1}' -f $n,$lines[$n-1] }`
> - Ran `$lines=Get-Content 'Versions/Temporary/Engine/Sources/3Dmotor/GView.cpp'; for($n=625;$n -le 705;$n++){ '{0,5}: {1}' -f $n,$lines[$n-1] }`
> - Ran `rg -n "void CGameView::CreateMeshInfo|CreateMeshInfo\(" Versions/Temporary/Engine/Sources/3Dmotor/GView.cpp Versions/Temporary/Engine/Sources/3Dmotor/GView.h`
> - Ran `$lines=Get-Content 'Versions/Temporary/Engine/Sources/3Dmotor/GView.cpp'; for($n=380;$n -le 445;$n++){ '{0,5}: {1}' -f $n,$lines[$n-1] }`
> - Ran `$lines=Get-Content 'Versions/Temporary/Engine/Sources/3Dmotor/GView.cpp'; for($n=440;$n -le 515;$n++){ '{0,5}: {1}' -f $n,$lines[$n-1] }`
> - Ran `rg --files | rg -i "\.(gr2|gltf|glb)$|granny" | Select-Object -First 300`
> - Ran `rg -n -C 8 "GrannyGetWorldPoseComposite4x4|GrannyBuildWorldPose|GrannySampleModelAnimations|GrannyBeginControlledAnimation|GrannySetSkeletonTrackMaskFromTrackGroup|struct granny_world_pose|struct granny_skeleton|struct granny_bone|struct granny_model" Versions/Temporary/Engine/Sources/vendor/granny/include/granny211.h | Select-Object -First 500`
> - Ran `Get-ChildItem 'Versions/Temporary/Engine/Sources/vendor/granny/include' -Force | Select-Object Name,Length; Get-ChildItem 'third_party' -Recurse -Filter granny211.h | Select-Object FullName,Length`
> - Ran `Get-ChildItem 'third_party' -Recurse -Filter granny211.h | ForEach-Object { $_.FullName }`
> - Ran `$i=1; Get-Content 'cmake/granny.cmake' | ForEach-Object { '{0,5}: {1}' -f $i++, $_ }; rg -n "granny.cmake|GRANNY_ROOT|common/granny" CMakeLists.txt cmake Versions/Temporary/Engine/Sources --glob '*.{cmake,txt}'`
> - Ran `rg -n -C 6 "GrannyGetWorldPoseComposite4x4|GrannyBuildWorldPose|GrannySampleModelAnimations|GrannyBeginControlledAnimation|GrannySetSkeletonTrackMaskFromTrackGroup|typedef struct granny_world_pose|struct granny_world_pose|typedef struct granny_bone|struct granny_bone" third_party/uesp-esoapps/common/granny/granny211.h | Select-Object -First 500`
> - Ran `rg -n "granny_bone|granny_skeleton|granny_transform|granny_model_instance|granny_track_group|granny_animation" third_party/uesp-esoapps/common/granny/granny211.h | Select-Object -First 160`
> - Ran `$lines=Get-Content 'third_party/uesp-esoapps/common/granny/granny211.h'; for($n=2255;$n -le 2300;$n++){ '{0,5}: {1}' -f $n,$lines[$n-1] }`
> - Ran `$lines=Get-Content 'third_party/uesp-esoapps/common/granny/granny211.h'; for($n=1795;$n -le 1825;$n++){ '{0,5}: {1}' -f $n,$lines[$n-1] }`
> - Ran `$lines=Get-Content 'third_party/uesp-esoapps/common/granny/granny211.h'; for($n=2555;$n -le 2638;$n++){ '{0,5}: {1}' -f $n,$lines[$n-1] }`
> - Ran `$lines=Get-Content 'third_party/uesp-esoapps/common/granny/granny211.h'; for($n=2080;$n -le 2140;$n++){ '{0,5}: {1}' -f $n,$lines[$n-1] }`
> - Ran `$lines=Get-Content 'third_party/uesp-esoapps/common/granny/granny211.h'; for($n=1400;$n -le 1475;$n++){ '{0,5}: {1}' -f $n,$lines[$n-1] }`
> - Ran `$lines=Get-Content 'Versions/Temporary/Engine/Sources/3Dmotor/aiObjectLoader.cpp'; for($n=1;$n -le 260;$n++){ '{0,5}: {1}' -f $n,$lines[$n-1] }`
> - Ran `$lines=Get-Content 'Versions/Temporary/Engine/Sources/3Dmotor/aiObjectLoader.h'; for($n=1;$n -le 130;$n++){ '{0,5}: {1}' -f $n,$lines[$n-1] }`
> - Ran `rg -n --glob '*.{h,hpp,cpp}' "public ISkeletonAnimator|CreateSkeletonAnimator\(|SGrannySkeletonPose|granny_local_pose" Versions/Temporary/Engine/Sources | Select-Object -First 500`
> - Ran `rg -n "SAnimHandle\s*[({]|SAnimHandle\b" Versions/Temporary/Engine/Sources --glob '*.{h,hpp,cpp}' | Select-Object -First 400`
> - Ran `rg -n "CreateFileRequiest|class CResourceFileOpener|GetBinaryFileName|GetExistentBinaryFileName|class CResourceOpener" Versions/Temporary/Engine/Sources/System Versions/Temporary/Engine/Sources/3Dmotor --glob '*.{h,hpp,cpp}' | Select-Object -First 300`
> - Ran `$i=1; Get-Content 'Versions/Temporary/Engine/Sources/System/BinaryResources.cpp' | ForEach-Object { '{0,5}: {1}' -f $i++, $_ }`
> - Ran `$lines=Get-Content 'Versions/Temporary/Engine/Sources/System/GResource.h'; for($n=1;$n -le 185;$n++){ '{0,5}: {1}' -f $n,$lines[$n-1] }`
> - Ran `rg -n -C 5 "CFileResource::CFileResource|CFileRequest::Read|CResourceFileOpener::DoesExist" Versions/Temporary/Engine/Sources/System --glob '*.cpp'`
> - Ran `$lines=Get-Content 'Versions/Temporary/Engine/Sources/System/GResource.cpp'; for($n=1;$n -le 165;$n++){ '{0,5}: {1}' -f $n,$lines[$n-1] }`
>
> </details>
>
> The architecture is a hybrid with a fairly clean seam at the renderer: Granny owns clip evaluation and pose construction, but the engine converts mesh data into its own vertex/index/weight structures and performs its own CPU skinning. That makes static glTF support much easier than replacing Granny for animated assets; I’m checking the asset pipeline and non-render consumers now, because those are where a “second format” often becomes unexpectedly broad.
>
> <details><summary>Ran 27 commands</summary>
>
> - Ran `rg -n -C 4 "GrannyRead|GrannyGetFileInfo|\.gr2|GetBinaryFileName|CopyFile|Animations\\|Skeletons\\|Geometries\\|Maya" Versions/Temporary/Engine/Sources/ED_B2_M1/AnimationBuilder.cpp Versions/Temporary/Engine/Sources/ED_B2_M1/SkeletonExporter.cpp Versions/Temporary/Engine/Sources/ED_B2_M1/ObjectBaseRPGStatsExporter.cpp Versions/Temporary/Engine/Sources/ED_Common/TempAttributesTool.cpp | Select-Object -First 600`
> - Ran `$lines=Get-Content 'Versions/Temporary/Engine/Sources/ED_B2_M1/AnimationBuilder.cpp'; for($n=100;$n -le 190;$n++){ '{0,5}: {1}' -f $n,$lines[$n-1] }`
> - Ran `$lines=Get-Content 'Versions/Temporary/Engine/Sources/ED_B2_M1/ObjectBaseRPGStatsExporter.cpp'; for($n=180;$n -le 235;$n++){ '{0,5}: {1}' -f $n,$lines[$n-1] }`
> - Ran `$lines=Get-Content 'Versions/Temporary/Engine/Sources/ED_B2_M1/SkeletonExporter.cpp'; for($n=1;$n -le 110;$n++){ '{0,5}: {1}' -f $n,$lines[$n-1] }`
> - Ran `rg -n "GetAddPath\(\) const|bin\\animations|bin\\geometries|ExportGeometry|REGISTER_EXPORTER_IN_DLL\( (Animation|Geometry)" Versions/Temporary/Engine/Sources/ED_B2_M1 Versions/Temporary/Engine/Sources/ED_Common --glob '*.{h,cpp}' | Select-Object -First 400`
> - Ran `$i=1; Get-Content 'Versions/Temporary/Engine/Sources/ED_Common/BasicSceneExporter.cpp' | ForEach-Object { '{0,5}: {1}' -f $i++, $_ }`
> - Ran `$i=1; Get-Content 'Versions/Temporary/Engine/Sources/ED_B2_M1/AnimationExporter.cpp' | ForEach-Object { '{0,5}: {1}' -f $i++, $_ }`
> - Ran `$i=1; Get-Content 'Versions/Temporary/Engine/Sources/ED_Common/BasicSceneExporter.h' | ForEach-Object { '{0,5}: {1}' -f $i++, $_ }`
> - Ran `$i=1; Get-Content 'Versions/Temporary/Engine/Sources/ED_Common/AIGeometryExporter.cpp' | ForEach-Object { '{0,5}: {1}' -f $i++, $_ }`
> - Ran `$i=1; Get-Content 'Versions/Temporary/Engine/Sources/ED_B2_M1/GeometryExporter.cpp' | ForEach-Object { '{0,5}: {1}' -f $i++, $_ }`
> - Ran `$i=1; Get-Content 'Versions/Temporary/Engine/Sources/3Dmotor/GrannyMemoryMap.cpp' | ForEach-Object { '{0,5}: {1}' -f $i++, $_ }; $i=1; Get-Content 'Versions/Temporary/Engine/Sources/3Dmotor/GrannyMemoryMap.hpp' | ForEach-Object { '{0,5}: {1}' -f $i++, $_ }`
> - Ran `rg -n "InitializeGrannyMemoryMap|DumpGrannyMemory" Versions/Temporary/Engine/Sources --glob '*.{h,cpp}'`
> - Ran `rg -n --glob '*.{h,hpp,cpp}' "GetMarkTimes\(|EnumMarks\(|GetChannelValue\(|GetChannelIndex\(|CreateAnimatedTransparencyChannels|SetGlobMoveAnimation|SetBoneMutator|SetSpecialMutator|CreateTransform\(" Versions/Temporary/Engine/Sources | Select-Object -First 500`
> - Ran `$lines=Get-Content 'Versions/Temporary/Engine/Sources/3Dmotor/GView.cpp'; for($n=1810;$n -le 1865;$n++){ '{0,5}: {1}' -f $n,$lines[$n-1] }; $i=1; Get-Content 'Versions/Temporary/Engine/Sources/3Dmotor/AnimatedChannel.h' | ForEach-Object { '{0,5}: {1}' -f $i++, $_ }`
> - Ran `$lines=Get-Content 'Versions/Temporary/Engine/Sources/3DLib/GGeometry.cpp'; for($n=240;$n -le 335;$n++){ '{0,5}: {1}' -f $n,$lines[$n-1] }`
> - Ran `$lines=Get-Content 'Versions/Temporary/Engine/Sources/3DLib/GGeometry.cpp'; for($n=130;$n -le 215;$n++){ '{0,5}: {1}' -f $n,$lines[$n-1] }`
> - Ran `$lines=Get-Content 'Versions/Temporary/Engine/Sources/3Dmotor/GLightmap.cpp'; for($n=205;$n -le 285;$n++){ '{0,5}: {1}' -f $n,$lines[$n-1] }`
> - Ran `rg -n "ReadAttributes\(|GetAttributesFromBone\(|CGrannyFileInfoGuard|GetGrannyAnimationLength|GetGrannyMeshBoundingBox" Versions/Temporary/Engine/Sources --glob '*.{h,cpp}' | Select-Object -First 500`
> - Ran `$lines=Get-Content 'Versions/Temporary/Engine/Sources/ED_B2_M1/ModelExporter.cpp'; for($n=40;$n -le 155;$n++){ '{0,5}: {1}' -f $n,$lines[$n-1] }`
> - Ran `$lines=Get-Content 'Versions/Temporary/Engine/Sources/ED_Common/Tools_Granny.cpp'; for($n=35;$n -le 90;$n++){ '{0,5}: {1}' -f $n,$lines[$n-1] }; for($n=200;$n -le 275;$n++){ '{0,5}: {1}' -f $n,$lines[$n-1] }`
> - Ran `rg -n --glob '*.{h,hpp,cpp}' "(pInfo|pFI|pData|fileInfo)->(Textures|Materials)|granny_material|granny_texture" Versions/Temporary/Engine/Sources/3Dmotor Versions/Temporary/Engine/Sources/SceneB2 Versions/Temporary/Engine/Sources/ED_Common Versions/Temporary/Engine/Sources/ED_B2_M1 | Select-Object -First 300`
> - Ran `rg --files Tools Versions/Current/Editor | rg -i "(granny|export).*(\.txt|\.cfg|\.ini|\.xml|\.mel|\.grn|\.gset)$|MayaExportSettings" | Select-Object -First 300`
> - Ran `Get-Content 'Tools/MayaExportSettings/settings_geom.ges' -TotalCount 220`
> - Ran `Get-Content 'Tools/MayaExportSettings/settings_anims.ges' -TotalCount 220`
> - Ran `Get-Content 'Tools/MayaExportSettings/B2ExportGeometry.mel' -TotalCount 220`
> - Ran `Get-Content 'Tools/MayaExportSettings/settings_skeleton.ges' -TotalCount 220`
> - Ran `Get-Content 'Tools/MayaExportSettings/B2ExportAnimation.mel' -TotalCount 220`
>
> </details>
>
> Context compaction
>
> Source: automatic
> Status: completed
>
> The main boundary is now clear: meshes are converted into engine-native data, but skeletal playback is deeply delegated to Granny. I’m doing one final consistency pass over the resource/export path and then I’ll lay out the practical migration options and their risks.
>
> <details><summary>Ran a command</summary>
>
> - Ran `git status --short; rg -n "GrannySampleModelAnimations|GrannyBuildWorldPose|GrannyGetWorldPoseComposite4x4|GrannyBeginControlledAnimation|class CSkeletonAnimator|public ISkeletonAnimator|GrannyReadEntireFileFromMemory|struct SWeight|MutateSkeletonPose" Versions/Temporary/Engine/Sources/3Dmotor Versions/Temporary/Engine/Sources/3DLib Versions/Temporary/Engine/Sources/SceneB2`
>
> </details>
>
> <details><summary>Plan</summary>
>
> Completed the read-only code inspection. The remaining work is the synthesized assessment in the response; no build or code changes were performed.
>
> - [x] Inventory Granny headers, loaders, and call sites
> - [x] Trace geometry, skeleton, animation, skinning, and render data flow
> - [x] Separate engine-owned behavior from Granny-owned behavior
> - [x] Assess incremental glTF integration paths and migration risks
>
> </details>
</details>

## Verdict

Granny is **not merely used as a `.gr2` file reader**. Blitzkrieg 2 uses Granny 2 as its low-level skeletal-animation runtime:

- Skeleton and animation resource decoding
- Animation-to-skeleton binding
- Animation controls
- Playback clocks, speed, duration, looping, and completion
- Fade-in/fade-out weighting
- Track sampling and blending
- Local pose generation
- Skeleton hierarchy evaluation
- Composite skinning-matrix generation

However, Granny does **not** own the whole animation or rendering system. The game wraps Granny in a substantial engine-level animation layer that handles gameplay animation state, root movement, animation annotations, procedural bone modification, save/load, attachments, collision, and ultimately rendering.

So the best characterization is:

> Blitzkrieg 2 has its own high-level animation system built around Granny as the skeletal pose evaluator.

There is currently no second, Granny-independent skeletal-animation backend.

## How the current pipeline works

```text
Extensionless game resource
          |
          v
Granny reads GR2 file and exposes file_info
          |
          +--> Mesh vertices/topology/skin weights
          |        |
          |        v
          |    Native CObjectInfo geometry
          |        |
          |        v
          |    Engine rendering and CPU skinning
          |
          +--> Skeleton + animation tracks
                   |
                   v
             Granny controls and sampling
                   |
                   v
             granny_local_pose
                   |
          Engine procedural mutators
                   |
                   v
        Granny hierarchy/world-pose building
                   |
                   v
         Native vector<SHMatrix>
                   |
        +----------+-----------+
        |                      |
   Rendered mesh          Animated collision,
   CPU skinning           locators/attachments
```

### Geometry loading

The game reads the complete resource into memory and passes it to `GrannyReadEntireFileFromMemory` in [GObjectInfo.cpp](../Versions/Temporary/Engine/Sources/3Dmotor/GObjectInfo.cpp#L51).

It then copies the Granny mesh into the engine’s own representation:

- Positions
- Normals
- Tangents and binormals
- First texture coordinate
- Triangle topology
- Material-group partitioning
- Bone indices and weights

That conversion is in [GObjectInfo.cpp](../Versions/Temporary/Engine/Sources/3Dmotor/GObjectInfo.cpp#L142). The destination is `CObjectInfo`, whose native vertex and skin-weight formats are defined in [GGeometry.h](../Versions/Temporary/Engine/Sources/3DLib/GGeometry.h#L13).

Important restrictions of the existing renderer include:

- At most four bone influences per vertex
- Bone indices stored as bytes, effectively limiting a skinning palette to 256 bones
- Triangle-based meshes
- The normal import path expects tangent and binormal data
- Only the first UV set is copied by the normal GR2 loader
- Mesh primitives/material groups must remain in the order expected by the model database

For skinned meshes, the loader maps Granny mesh bone bindings to the selected external skeleton by **bone name**. This means geometry and skeleton resources can be stored independently. See [GObjectInfo.cpp](../Versions/Temporary/Engine/Sources/3Dmotor/GObjectInfo.cpp#L235).

Once converted, Granny no longer participates in drawing the mesh. Granny materials and textures are effectively ignored; the game’s DB model records and engine material system supply those separately in [DBScene.h](../Versions/Temporary/Engine/Sources/3Dmotor/DBScene.h#L33).

### Skeletal animation

The animation side remains much more deeply tied to Granny.

`CSkeletonAnimator` creates a Granny model instance around the selected skeleton in [GAnimation.cpp](../Versions/Temporary/Engine/Sources/3Dmotor/GAnimation.cpp#L161).

When an animation is added, it uses Granny’s controlled-animation API:

- `GrannyBeginControlledAnimation`
- Track-group binding to the model
- Track masking
- Speed and looping configuration
- Ease-in and ease-out
- Completion and end-time handling

That path begins in [GAnimation.cpp](../Versions/Temporary/Engine/Sources/3Dmotor/GAnimation.cpp#L453).

Every pose update eventually calls `GrannySampleModelAnimations`, producing a `granny_local_pose` in [GAnimation.cpp](../Versions/Temporary/Engine/Sources/3Dmotor/GAnimation.cpp#L336).

The game then applies its own additions:

- Root/global movement correction
- Scalar animation channels
- Procedural per-bone transformations
- Special object-specific pose mutators

Finally, Granny constructs the hierarchical world pose and composite skinning matrices in [GBind.cpp](../Versions/Temporary/Engine/Sources/3Dmotor/GBind.cpp#L51).

Those composite matrices account for the skeleton hierarchy and inverse bind matrices.

### Rendering and skinning

The resulting matrices are converted into the engine’s `SSkeletonMatrices`. The renderer performs the actual vertex skinning itself.

The CPU linear-blend skinning implementation is in [GCombiner.cpp](../Versions/Temporary/Engine/Sources/3Dmotor/GCombiner.cpp#L211). It blends up to four bone matrices per vertex, transforms positions and normal bases, and uploads the transformed vertices to a dynamic buffer.

The same engine matrix representation is also used to deform animated collision/AI geometry in [aiObjectLoader.cpp](../Versions/Temporary/Engine/Sources/3Dmotor/aiObjectLoader.cpp#L171).

This separation is encouraging for glTF support: the renderer already consumes native geometry plus a vector of matrices. It does not need Granny objects directly.

## What belongs to Granny versus the game

| Responsibility | Owner |
|---|---|
| Decode GR2 skeletons, meshes, and animations | Granny |
| Animation curve representation and sampling | Granny |
| Clip-to-skeleton binding | Granny |
| Animation controls, clocks, speed, looping | Granny |
| Ease-in/ease-out and pose blending | Granny |
| Skeleton hierarchy/world-pose evaluation | Granny |
| Composite skinning matrices | Granny |
| Selecting gameplay animations | Game |
| Gameplay animation metadata | Game DB |
| Root movement behavior | Game |
| Procedural bone modification | Game |
| Animation save/load state | Game |
| Named scalar-channel mixing | Game, sampling Granny curves |
| Materials and textures | Game |
| CPU vertex skinning and rendering | Game |
| Animated collision hulls | Game |
| Bone locators and attachments | Game using Granny-produced poses |

The public animation abstraction is not cleanly format-independent. `IAnimMutator` directly exposes `granny_local_pose*` in [GAnimation.hpp](../Versions/Temporary/Engine/Sources/3Dmotor/GAnimation.hpp#L30).

Several gameplay modules depend on that:

- [MechUnitJoggingMutator.cpp](../Versions/Temporary/Engine/Sources/SceneB2/MechUnitJoggingMutator.cpp#L65)
- [TreeWindMutator.cpp](../Versions/Temporary/Engine/Sources/SceneB2/TreeWindMutator.cpp#L24)
- [TreeFallingMutator.cpp](../Versions/Temporary/Engine/Sources/SceneB2/TreeFallingMutator.cpp#L89)
- [WingScaleMutator.cpp](../Versions/Temporary/Engine/Sources/SceneB2/WingScaleMutator.cpp#L50)

The mutator interface is not the widest leak, though. `ISkeletonAnimator` derives from `CFuncBase<SGrannySkeletonPose>`, and `SGrannySkeletonPose` is itself a Granny type in all but name:

```cpp
struct SGrannySkeletonPose
{
    float poseGlobal[16];
    granny_local_pose *pPose;
};
```

See [GSkeleton.h](../Versions/Temporary/Engine/Sources/3Dmotor/GSkeleton.h#L29). So the *value the animator publishes* is a `granny_local_pose*`, not just the argument the mutators receive. Neutralizing `IAnimMutator` alone does not produce a format-independent seam; this struct has to be replaced at the same time, and it is what `GAnimUtils`, `GBind` and every pose consumer read.

Furthermore, `CSkeletonAnimator` is the only implementation of `ISkeletonAnimator` I found. So the interface resembles an abstraction, but the implementation and some consumers still expose Granny types.

### Three GR2 readers, not one

The diagram above shows the `CObjectInfo` path, which is the main one but not the only one. Two further paths parse `granny_mesh` themselves and never produce a `CObjectInfo`:

- **AI/collision geometry.** `CLoadAIGeometryFromGranny` and `CFileSkinPointsLoadFromGranny` in [aiObjectLoader.cpp](../Versions/Temporary/Engine/Sources/3Dmotor/aiObjectLoader.cpp#L171) walk `granny_file_info->Meshes[]` directly and call `ConvertAIGeomVerticesFromGranny` / `ConvertAIGeomTrisFromGranny`, plus `ConvertWeightsFromGrannyEx` for the skinned variant.
- **Terrain/debris meshes.** [TerraTools.cpp](../Versions/Temporary/Engine/Sources/SceneB2/TerraTools.cpp#L68) has its own `GetGrannyTypedefOffset` and a `LoadGrannyModel` that returns bare vertices and triangles. It is live in the game, not editor-only: [DebrisBuilder.cpp](../Versions/Temporary/Engine/Sources/SceneB2/DebrisBuilder.cpp#L119) calls it, alongside the editor's `PassabilityProfile.cpp`.

This matters for scoping: "static glTF models load" is not true across the game until all three are covered.

### Save-format coupling

`SAnimHandle` and `SGrannySkeletonHandle` are `IBinSaver` types with fixed chunk ids ([GAnimation.hpp](../Versions/Temporary/Engine/Sources/3Dmotor/GAnimation.hpp#L19), [GSkeleton.h](../Versions/Temporary/Engine/Sources/3Dmotor/GSkeleton.h#L36)). Both serialize a `CDBPtr` to a DB record plus an index *into the GR2 file* (`nAnimNumber`, `nModelInFile`). A second backend has to either keep those chunks meaning the same thing or version them; per the repo's serialization rules, the ids themselves are never renumbered or reused.

## What the game adds above Granny

The game has meaningful animation functionality of its own.

Animation database entries carry gameplay-specific information such as:

- Animation type
- Action time
- Nominal length
- Looping
- Weapon-related metadata
- Animated bounding boxes
- Movement speed

Those records are in [DBAnimB2.h](../Versions/Temporary/Engine/Sources/Stats_B2_M1/DBAnimB2.h#L37).

The engine also:

- Maintains its own list of active animations
- Serializes their logical state and reconstructs Granny controls on load
- Implements custom global/root movement
- Extracts text-track annotations as event marks
- Samples arbitrary named vector/scalar tracks
- Uses channels such as `<mesh-name>.Transparency`
- Applies engine-defined bone mutators after normal animation sampling
- Uses animated bones for effect locators, attachments, bounds, and collision

Therefore, replacing Granny animation does not require replacing all game animation logic. It requires reproducing the lower layer well enough that the existing high-level logic behaves identically.

## Resource and exporter considerations

The game’s VFS resources do not normally retain extensions. A resource is loaded from a path such as `bin\geometries\<UID>` in [GResource.cpp](../Versions/Temporary/Engine/Sources/System/GResource.cpp#L34).

Consequently, adding `.gltf` versus `.gr2` extension dispatch is not sufficient. You would need one of:

- A database format field
- A magic/header discriminator
- Separate resource categories
- A new native compiled-resource version

For new content I would strongly favor **GLB as authoring input**, followed by offline conversion to a compact engine-native binary. External `.gltf` buffers and texture references fit the current single-resource VFS rather poorly.

The editor/exporter is also Granny-oriented. Geometry, skeleton, animation, and AI-geometry exporters run Granny’s Maya exporter and then store the resulting GR2 bytes under extensionless resource names. For example:

- [GeometryExporter.cpp](../Versions/Temporary/Engine/Sources/ED_B2_M1/GeometryExporter.cpp#L11)
- [AnimationExporter.cpp](../Versions/Temporary/Engine/Sources/ED_B2_M1/AnimationExporter.cpp#L12)
- [SkeletonExporter.cpp](../Versions/Temporary/Engine/Sources/ED_B2_M1/SkeletonExporter.cpp#L21)

The runtime performs essentially raw coordinate copies from Granny. That suggests the existing Maya/Granny exporter bakes the engine’s expected coordinate convention. A glTF importer will need explicit axis, handedness, winding, scale, quaternion, tangent-sign, and inverse-bind conversions. Those conversions should be calibrated against a known existing asset rather than guessed.

## Feasibility by scope

### Static glTF models: quite feasible

This is the easiest and cleanest first milestone.

A glTF/GLB loader or offline compiler can produce the same `CObjectInfo` data that the Granny mesh loader currently creates. Everything after that—materials, culling, rendering, and collision—can remain unchanged.

There is even precedent for loading native serialized `CObjectInfo::SBinData` before falling back to Granny in [GObjectInfo.cpp](../Versions/Temporary/Engine/Sources/3Dmotor/GObjectInfo.cpp#L436), although the current lightmap cache format should probably not be repurposed unchanged as the final asset format.

### glTF skinned meshes using an existing Granny skeleton: possible but fragile

You could import only the mesh and map its joints by name to a legacy Granny skeleton.

This will work only if:

- Bone names are identical and unique
- Hierarchy and rest pose match
- Coordinate conversions match exactly
- The glTF inverse-bind matrices agree with the legacy skeleton

The current binding path ultimately uses the selected Granny skeleton’s inverse-bind information. It would not automatically honor different `inverseBindMatrices` supplied by the glTF skin. Therefore this is safe primarily for models authored against the exact legacy skeleton.

### Native glTF skeletons and animation: feasible, but a substantial project

This is not just adding another geometry decoder. A new backend must supply equivalents for:

- Skeleton hierarchy and rest pose
- STEP, LINEAR, and CUBICSPLINE keyframe sampling
- Quaternion interpolation and normalization
- Missing-channel/rest-pose behavior
- Clip binding
- Looping and clocks
- Playback speed
- Fade curves and weighted mixing
- Completion semantics
- World transforms and inverse-bind skin matrices
- Procedural mutators
- Text/event annotations
- Named scalar channels
- Save/load reconstruction

The renderer would largely survive unchanged. The animation runtime is the part that needs refactoring.

### Eliminating Granny completely: hard, but the surface is smaller than it looks

Keeping Granny as a legacy backend is realistic. On Windows the build imports the proprietary DLL directly ([granny.cmake](../cmake/granny.cmake)), so it still has to ship wherever legacy GR2 files are supported.

Removing the DLL while continuing to consume arbitrary legacy GR2 assets would require a compatible GR2 reader and animation decoder, or a one-time conversion of all legacy assets. That sounds like reimplementing Granny. It is much narrower than that, and the Linux port has already measured how much narrower.

**The tree needs 54 entry points, not the several hundred `granny211.h` declares.** `cmake/granny.cmake` now has a non-Windows branch that links [GrannyStub.cpp](../Versions/Temporary/Engine/Sources/vendor/granny/GrannyStub.cpp) instead, defining exactly those 54 as no-ops. That is the whole link-time surface:

```
Decode      ReadEntireFile{,FromMemory} GetFileInfo FreeFile MeshIsRigid
            GetMeshTriangleGroupCount GetTotalObjectSize GetMemberTypeSize
            FindBoneByName GetAllocator SetAllocator
Instance    InstantiateModel FreeModelInstance SetModelClock
Controls    BeginControlledAnimation EndControlledAnimation FreeControl
            FreeControlOnceUnused SetControlActive SetControlSpeed GetControlSpeed
            SetControlRawLocalClock GetControlClampedLocalClock GetControlDuration
            GetControlDurationLeft GetControlEffectiveWeight SetControlLoopCount
            SetControlForceClampedLooping CompleteControlAt ControlIsComplete
            EaseControlIn EaseControlOut SetControlEaseIn SetControlEaseOut
            SetControlEaseInCurve SetControlEaseOutCurve
Tracks      SetTrackGroupTarget SetTrackGroupAccumulation SetTrackGroupModelMask
            NewTrackMask SetSkeletonTrackMaskFromTrackGroup
Sampling    SampleModelAnimations EvaluateCurveAtT
Pose        NewLocalPose FreeLocalPose GetLocalPoseBoneCount GetLocalPoseTransform
            NewWorldPose FreeWorldPose BuildWorldPose GetWorldPose4x4
            GetWorldPoseComposite4x4 MakeIdentity PostMultiplyBy
```

Two things are worth reading off that list. There is **nothing for materials, textures, lights, cameras or drawing** - which independently confirms the ownership table above. And the shape of a replacement is visible in it: a `.gr2` parser, a keyframe evaluator, a controls/clock layer, and a hierarchy walk.

**A reference is not a call, and the surface is likely narrower still.** Some of the 54 sit on paths the shipped data cannot reach, and a linker cannot tell the difference. So each stub records that it was called, with a global ordinal, its arguments (`char const*` logged as the string, since that names the model), and a per-function count dumped at exit, to `granny_calls.log`.

The first run that reached a model produced a three-line log: `GrannyReadEntireFileFromMemory` twice and `GrannyGetFileInfo` once, the last of which is already the crash - the stub returned null, and the caller dereferenced the result. **That is a floor, not the answer**: the log stops because the process does, so it says which entry point to implement *first* and nothing yet about the other 52. The narrowing only becomes real once a failed load degrades instead of crashing and a longer log can be read. Details and the current state are in [PORT_ROADMAP.md](port/PORT_ROADMAP.md#granny-and-the-log-that-says-what-to-port).

There is a second, weaker narrowing available on top of the count: an entry point that is only ever called with one shape of argument does not need a general implementation. `GrannySetTrackGroupAccumulation` is only ever passed `GrannyNoAccumulation` ([GAnimation.cpp](../Versions/Temporary/Engine/Sources/3Dmotor/GAnimation.cpp#L457)), for instance, so accumulation modes need not exist at all. The argument log is what makes that checkable per function rather than guessed.

## Recommended architecture

I would avoid making `CSkeletonAnimator` directly understand both Granny and glTF. Instead, introduce a neutral layer:

1. Define native skeleton data:

   - Bone name
   - Parent index
   - Rest local transform
   - Inverse bind matrix

2. Define a neutral local-pose representation. Procedural mutators should receive this rather than `granny_local_pose*`.

3. Define native animation clips and event/scalar tracks.

4. Have animators produce engine `SSkeletonMatrices`, or a neutral pose from which those matrices can be built.

5. Keep two backends:

   - Legacy Granny backend
   - Native/glTF backend

6. Keep the current renderer, CPU skinner, material system, collision skinning, and most gameplay animation control code.

7. Compile GLB files offline into engine-oriented resources rather than parsing full glTF at every game load.

A transitional implementation could retain Granny internally for legacy assets while translating its output into the neutral pose/matrix API. New assets would never need to become Granny objects.

## glTF metadata that needs special treatment

Standard glTF does not directly represent several conventions the game relies on:

- Named text/event tracks
- Arbitrary named scalar channels such as mesh transparency
- Game animation type/action time/movement speed
- Per-animation bounding boxes
- Mesh animated/wind-affected flags
- The existing external DB material assignments
- AI/collision geometry
- The engine’s geometry-part/material-part organization

These should live in the existing DB, a sidecar, or a defined glTF `extras`/custom-extension schema. Importing standard glTF PBR materials alone will not make them work with the current renderer.

## Bottom line

Adding glTF support is very plausible because the rendering half is already largely format-independent.

- **Static glTF support:** good fit and moderate work, provided all three GR2 read paths are covered and not just `CObjectInfo`.
- **glTF meshes on existing legacy skeletons:** possible, but requires strict bind-pose compatibility.
- **Fully native glTF animation:** feasible, but requires a real alternative skeletal-animation backend.
- **Keeping Granny only for legacy:** a sensible architecture.
- **Removing Granny entirely while retaining GR2 compatibility:** still the hardest of these, but bounded: 54 entry points, none of them touching materials or rendering, and probably fewer once the call log runs past the first model.

The key insight is that the proper seam is not merely “GR2 loader versus glTF loader.” It is:

> native geometry + neutral skeleton/pose/clip interfaces, with Granny retained as one legacy implementation.

No code was changed or built. The working tree already contained unrelated modifications before/throughout this inspection, which I left untouched.
