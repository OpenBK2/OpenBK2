# GLB format usage

In `ExampleDinosaurUnit` folder, you'll find the animated dinosuar model and its configuration for the game.

In the current state of the game, only GLB files are supported. Don't bloat them with texture or materials - the game has its own system for them!

For Geometry, AIGeometry and Skeleton configurations, only the `RootJoint` property object and its children will be taken into the game. So you can store both AABB and main model in the same GLB file. E.g. For Geometry and Skeleton, the `RootJoint` can be `Basis`. While the AIGeometry can have `RootJoint` as `AABB`.

When doing animation clip splitting, either have all animations in one big clip in GLB (that's when you use FirstFrame and LastFrame properties), or have all of them in their own separate clips (aka actions in Blender) - that's when you use ClipName property. Setting both at once is not supported: a non-empty ClipName selects the whole clip and the frame range is ignored. That combination is reported in the log, along with the two other ways frame slicing can fail, a GLB exported without animation sampling and a range outside the timeline the exporter baked. This is still an improvement from the old maya where all clips had to be in one sequence and then exported to separate GR2 files. 

Locators should probably be rotated 90 degrees along the X axis to show effects properly.