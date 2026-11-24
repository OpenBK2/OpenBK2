# Blender smoke puff

Open `Smoke.blend` in Blender 5.3 or newer. The **Smoke Puff** collection contains one ordinary emitter. It produces 18 growing, fading sprites in a two-second clip at 30 fps. The two particle instances in the example effect play that clip at different speeds, scales and offsets, with their two stock dust textures assigned in opposite order.

## Preview it in the Map Editor

1. Copy the contents of this example's `Data` folder into your mod's data root. The included UID file allows immediate playback using the game's stock dust textures.
2. Open the mod and select `Effects/Examples/BlenderSmoke/Smoke_Effect.xdb` in the **Effect** tree. Use the effect preview to see both layers.

## Change it in Blender

1. Install the [Blender add-on](../../BlenderPlugins/ParticleExport/README.md) and open `Smoke.blend`.
2. Select **Smoke emitter**. Under **Particle Properties > BK2 Sprite Appearance**, try a larger end size or a different end color. To change the emission or physics, free the saved particle bake under **Cache**, edit the settings, then bake again.
3. Choose **File > Export > Blitzkrieg 2 Particle Clip (.ptc)**, select **Smoke Puff**, and write over your mod's `Effects/Examples/BlenderSmoke/Smoke.ptc`.
4. In the Map Editor export **Smoke_Particle.xdb** and save changes. Reopen the effect preview. The editor installs the new bytes under the same UID and calculates the new bounds. Both instances now use the edited clip.

The editable source is [Smoke.blend](Smoke.blend). The example's [Effect](Data/Effects/Examples/BlenderSmoke/Smoke_Effect.xdb), [first instance](Data/Effects/Examples/BlenderSmoke/Smoke_ParticleInstance.xdb), [second instance](Data/Effects/Examples/BlenderSmoke/Drift_ParticleInstance.xdb), and [Particle](Data/Effects/Examples/BlenderSmoke/Smoke_Particle.xdb) show the complete reference chain.

## Try the Geometry Nodes wisp

In the Outliner, tick the checkbox beside the separate **Geometry Nodes Wisp** collection to enable it in the view layer. Select **Drifting wisp (Geometry Nodes)** and open its node graph to see how one point with a stable ID becomes a rising sprite. To export that example on its own, choose **Geometry Nodes Wisp** in the export dialog and save it as `Wisp.ptc`.
