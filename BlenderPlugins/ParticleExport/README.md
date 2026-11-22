# Blender particle export for Blitzkrieg 2

Install `bk2_particle_export.py` from **Edit > Preferences > Add-ons > Install from Disk**, then enable **Blitzkrieg 2 Particle Export**. The add-on uses Blender 4.5 or newer. The supplied example was created with Blender 5.3.

## Export a clip

1. Put the emitters or Geometry Nodes objects for one BK2 clip in a collection. Child collections are included. Objects must be available in the current view layer.
2. Set the scene frame range and FPS. The first scene frame becomes frame zero in the clip. Export uses one sample per frame at the scene's actual rate, including the FPS base. The format allows up to 32767 sampled frames.
3. Choose **File > Export > Blitzkrieg 2 Particle Clip (.ptc)**. Select the collection, choose a filename, and export. The collection's **Instance Offset** specifies the origin. At the default scale one Blender unit equals one game rendering unit. Positions follow the existing Blender-to-BK2 model convention, with Z up and both horizontal axes reversed.
4. Place the clip in the active mod or configured source folder. In the Map Editor, select it in the **Particle** record's **SrcName**, export the record, and save the database changes. The editor writes the UID file and calculates **Bound**.
5. Reference the particle from a **ParticleInstance**, assign **Textures** in slot order, then add the instance to an **Effect**. Use a separate collection and clip for layers that need independent playback. Multiple instances can reuse one clip.

The legacy Maya `ExportPrefix` remains in existing XDBs. Each Blender clip already contains the chosen collection.

### Blender emitters

Use a regular **Emitter** particle system. Its simulated world positions, lifetimes and sizes drive the bake. **Particle Properties > BK2 Sprite Appearance** provides birth/end RGBA, width and height multipliers, initial twist, spin speed, and first/last texture slots. The multipliers apply to the particle diameter. Color and size interpolate across each particle's own lifetime. The sprite advances through the inclusive slot range during that lifetime. Set both slots to the same number for a fixed image.

The exporter samples evaluated parent particles in the viewport. Set the viewport display percentage to 100% for the full effect. For reproducible physics, bake the particle cache before export. An unbaked emitter can also be exported by stepping the simulation. BK2 draws textured billboards. Its images and blending come from the instance's Texture XDBs.

### Geometry Nodes

Output a point cloud, or a mesh with one vertex per particle, from the modifier. Export the points before instancing display geometry onto them. Baking a Simulation Zone makes its evaluated positions repeatable. Store the following named attributes on the **Point** domain.

| Attribute | Blender type | Meaning |
|---|---|---|
| `id` | Integer | Required stable ID unique within the object's point cloud or mesh. Keep it unchanged throughout a particle's life, even when other points disappear or change order. |
| `radius` | Float | Radius in object space. The default sprite is one unit across when neither size attribute is present. |
| `bk2_size` | 2D Vector or Vector | Full sprite width and height. Overrides radius. A 3D vector uses X and Y. |
| `bk2_color` | Color or Byte Color | RGBA tint, default opaque white. |
| `bk2_twist` | Float | Billboard twist in radians, default zero. |
| `bk2_sprite` | Integer | Zero-based index into the instance's Textures array, default zero. |

Each continuous run of frames on which an ID exists becomes one particle. Two simultaneous points with the same ID produce an export error. Color, size, twist and position can all change independently per frame.

[Working smoke example](../../OBK2_Examples/BlenderParticleExport/README.md)

To run the export regression from the repository root, use Blender 5.3 or newer:

```sh
blender --background --factory-startup --python-exit-code 1 --python BlenderPlugins/ParticleExport/tests/blender_export.py
```
