"""Run with blender --background --factory-startup --python-exit-code 1 --python <this file>."""

from pathlib import Path
import struct
import sys
import tempfile

import bpy

addon_dir = Path(__file__).resolve().parents[1]
example = addon_dir.parents[1] / "OBK2_Examples/BlenderParticleExport"
sys.path.insert(0, str(addon_dir))
import bk2_particle_export as addon


def check_export(folder):
    scene = bpy.context.scene
    scene.frame_set(38, subframe=0.25)
    output = folder / "smoke.ptc"

    def export(collection):
        status = bpy.ops.export_scene.bk2_particle(collection=collection, filepath=str(output))
        assert status == {"FINISHED"}
        assert (scene.frame_current, scene.frame_subframe) == (38, 0.25)
        return output.read_bytes()

    # The saved emitter bake must reproduce both copies of the clip shipped with the example.
    assert bpy.context.collection.name == "Smoke Puff"
    smoke = export("Smoke Puff")
    size, duration, fps, count = struct.unpack_from("<iffi", smoke)
    assert (size, duration, fps, count) == (len(smoke) - 4, 2, 30, 18)
    assert smoke == (example / "Data/Effects/Examples/BlenderSmoke/Smoke.ptc").read_bytes()
    assert smoke == (example / "Data/bin/Effects/b70c16df-f18d-4152-ae05-5b04d9648c33").read_bytes()

    wisp_layer = bpy.context.view_layer.layer_collection.children["Geometry Nodes Wisp"]
    assert wisp_layer.exclude
    wisp_layer.exclude = False
    wisp = export(wisp_layer.name)
    assert struct.unpack_from("<ihh", wisp, 12) == (1, 0, 60)
    assert export("Smoke Puff") == smoke

    # Selecting their common parent explicitly includes both particle sources.
    combined = export(scene.collection.name)
    assert struct.unpack_from("<i", combined, 12)[0] == 19

    # A failed export must preserve the timeline and the previous clip.
    empty = bpy.data.collections.new("Empty")
    scene.collection.children.link(empty)
    try:
        export(empty.name)
    except RuntimeError as error:
        assert "No live emitter particles" in str(error)
    else:
        raise AssertionError("Empty export should fail")
    assert output.read_bytes() == combined
    assert (scene.frame_current, scene.frame_subframe) == (38, 0.25)


bpy.ops.wm.open_mainfile(filepath=str(example / "Smoke.blend"))
addon.register()
try:
    with tempfile.TemporaryDirectory() as directory:
        check_export(Path(directory))
finally:
    addon.unregister()
