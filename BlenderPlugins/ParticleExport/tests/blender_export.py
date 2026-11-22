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

    # The example carries its physics bake so consecutive exports can be compared.
    combined = export("Smoke Puff")
    assert combined == export("Smoke Puff")
    size, duration, fps, count = struct.unpack_from("<iffi", combined)
    assert (size, duration, fps, count) == (len(combined) - 4, 2, 30, 19)
    for name, expected in (("Smoke emitter", 18), ("Drifting wisp (Geometry Nodes)", 1)):
        collection = bpy.data.collections.new(name + " only")
        scene.collection.children.link(collection)
        collection.objects.link(bpy.data.objects[name])
        data = export(collection.name)
        assert struct.unpack_from("<i", data, 12)[0] == expected

    # A failed export must preserve the timeline and the previous clip.
    empty = bpy.data.collections.new("Empty")
    scene.collection.children.link(empty)
    try:
        export(empty.name)
    except RuntimeError as error:
        assert "No live emitter particles" in str(error)
    else:
        raise AssertionError("Empty export should fail")
    assert output.read_bytes() == data
    assert (scene.frame_current, scene.frame_subframe) == (38, 0.25)


bpy.ops.wm.open_mainfile(filepath=str(example / "Smoke.blend"))
addon.register()
try:
    with tempfile.TemporaryDirectory() as directory:
        check_export(Path(directory))
finally:
    addon.unregister()
