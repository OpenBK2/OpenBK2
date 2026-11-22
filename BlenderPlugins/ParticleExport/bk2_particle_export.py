"""Export a Blender collection as a native Blitzkrieg 2 sprite animation."""

bl_info = {
    "name": "Blitzkrieg 2 Particle Export",
    "author": "OpenBK2 contributors",
    "version": (1, 0, 0),
    "blender": (4, 5, 0),
    "location": "File > Export > Blitzkrieg 2 Particle Clip",
    "category": "Import-Export",
}

import math
import os
import struct
import tempfile

import bpy
from bpy.props import FloatProperty, FloatVectorProperty, IntProperty, PointerProperty, StringProperty
from bpy_extras.io_utils import ExportHelper


class BK2ParticleAppearance(bpy.types.PropertyGroup):
    start_color: FloatVectorProperty(name="Birth Color", subtype="COLOR", size=4,
                                     min=0, max=1, default=(1, 1, 1, 1))
    end_color: FloatVectorProperty(name="End Color", subtype="COLOR", size=4,
                                   min=0, max=1, default=(1, 1, 1, 1))
    start_size: FloatVectorProperty(name="Birth Size Multiplier", size=2, min=0, default=(1, 1))
    end_size: FloatVectorProperty(name="End Size Multiplier", size=2, min=0, default=(1, 1))
    twist: FloatProperty(name="Initial Sprite Angle", subtype="ANGLE")
    spin: FloatProperty(name="Spin per Second", subtype="ANGLE")
    first_sprite: IntProperty(name="First Texture Slot", min=0, max=32767)
    last_sprite: IntProperty(name="Last Texture Slot", min=0, max=32767)


class BK2_PT_emitter(bpy.types.Panel):
    bl_label = "BK2 Sprite Appearance"
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "particle"

    @classmethod
    def poll(cls, context):
        system = context.particle_system
        return system and system.settings.type == "EMITTER"

    def draw(self, context):
        appearance = context.particle_system.settings.bk2_appearance
        for field in ("start_color", "end_color", "start_size", "end_size",
                      "twist", "spin", "first_sprite", "last_sprite"):
            self.layout.prop(appearance, field)


def packed_color(rgba):
    if not all(math.isfinite(x) for x in rgba):
        raise ValueError("Sprite color must be finite")
    r, g, b, a = (round(255 * min(1, max(0, x))) for x in rgba)
    return (a << 24) | (r << 16) | (g << 8) | b


def emitter_samples(obj, frame, fps):
    for system in obj.particle_systems:
        if system.settings.type != "EMITTER":
            continue
        style = system.settings.bk2_appearance
        slots = style.last_sprite - style.first_sprite
        for index, particle in enumerate(system.particles):
            if not particle.is_exist or particle.alive_state != "ALIVE":
                continue
            elapsed = max(0, frame - particle.birth_time)
            age = min(1, elapsed / max(particle.lifetime, 1e-6))
            color = tuple(a + (b - a) * age for a, b in zip(style.start_color, style.end_color))
            size = tuple(2 * particle.size * (a + (b - a) * age)
                         for a, b in zip(style.start_size, style.end_size))
            step = min(abs(slots), int(age * (abs(slots) + 1)))
            slot = style.first_sprite + (step if slots >= 0 else -step)
            key = (obj.name, system.name, index, particle.birth_time)
            yield key, particle.location, style.twist + style.spin * elapsed / fps, size, color, slot


def attribute(geometry, name, types, required=False):
    attr = geometry.attributes.get(name)
    if attr is None:
        if required:
            raise ValueError(f"Set a stable integer '{name}' on the Geometry Nodes points")
        return None
    if attr.domain != "POINT" or attr.data_type not in types:
        raise ValueError(f"'{name}' must be a point-domain {' or '.join(types)} attribute")
    return attr.data


def node_samples(obj):
    if obj.type != "POINTCLOUD" and not any(m.type == "NODES" and m.show_viewport for m in obj.modifiers):
        return
    result = obj.evaluated_geometry()
    for kind, geometry in (("points", result.pointcloud), ("mesh", result.mesh)):
        if geometry is None:
            continue
        points = geometry.points if kind == "points" else geometry.vertices
        if not len(points):
            continue
        # The surface of an emitter or a mesh accompanying a point cloud is not a second emitter.
        if kind == "mesh" and "id" not in geometry.attributes and (obj.particle_systems or result.pointcloud):
            continue
        ids = attribute(geometry, "id", ("INT",), required=True)
        sizes = attribute(geometry, "bk2_size", ("FLOAT2", "FLOAT_VECTOR"))
        radii = attribute(geometry, "radius", ("FLOAT",))
        colors = attribute(geometry, "bk2_color", ("FLOAT_COLOR", "BYTE_COLOR"))
        twists = attribute(geometry, "bk2_twist", ("FLOAT",))
        slots = attribute(geometry, "bk2_sprite", ("INT",))
        scale = obj.matrix_world.to_scale()
        for i, point in enumerate(points):
            diameter = 2 * radii[i].value if radii is not None else 1.0
            width, height = sizes[i].vector[:2] if sizes is not None else (diameter, diameter)
            key = (obj.name, "GN", kind, ids[i].value)
            yield (key, obj.matrix_world @ point.co, twists[i].value if twists is not None else 0,
                   (width * scale.x, height * scale.y), colors[i].color if colors is not None else (1, 1, 1, 1),
                   slots[i].value if slots is not None else 0)


def bake(context, collection, unit_scale):
    scene = context.scene
    length = scene.frame_end - scene.frame_start + 1
    if not 1 <= length <= 32767:
        raise ValueError("The scene range must contain between 1 and 32767 frames")
    fps = scene.render.fps / scene.render.fps_base
    origin = collection.instance_offset.copy()
    objects = sorted((o for o in collection.all_objects if o.name in context.view_layer.objects),
                     key=lambda o: o.name)
    # Unbaked emitter caches need consecutive evaluation, even before the export range.
    warmup = min([scene.frame_start] + [ps.point_cache.frame_start for o in objects
                 for ps in o.particle_systems if ps.settings.type == "EMITTER" and not ps.point_cache.is_baked])
    original_frame, subframe = scene.frame_current, scene.frame_subframe
    clips = []
    previous = {}
    try:
        for frame in range(warmup, scene.frame_end + 1):
            scene.frame_set(frame)
            depsgraph = context.evaluated_depsgraph_get()
            if frame < scene.frame_start:
                continue
            tick = frame - scene.frame_start
            active = {}
            for obj in objects:
                evaluated = obj.evaluated_get(depsgraph)
                for source in (emitter_samples(evaluated, frame, fps), node_samples(evaluated)):
                    for key, pos, twist, size, color, slot in source:
                        if key in active:
                            raise ValueError(f"{obj.name}, frame {frame}: duplicate particle ID {key[-1]}")
                        if not 0 <= slot <= 32767:
                            raise ValueError(f"{obj.name}, frame {frame}: invalid texture slot {slot}")
                        # Blender and BK2 both use Z up. Match Blender -> glTF -> BK2 on the ground plane.
                        x, y, z = (pos - origin) * unit_scale
                        sample = ((-x, -y, z), (twist,), tuple(v * unit_scale for v in size),
                                  (packed_color(color),), (slot,))
                        if not all(math.isfinite(v) for channel in sample[:3] for v in channel):
                            raise ValueError(f"{obj.name}, frame {frame}: non-finite particle sample")
                        segment = previous.get(key)
                        if segment is None:
                            segment = []
                            clips.append(segment)
                        segment.append((tick, sample))
                        active[key] = segment
            previous = active
    finally:
        scene.frame_set(original_frame, subframe=subframe)
    if not clips:
        raise ValueError("No live emitter particles or Geometry Nodes points in the collection and scene range")
    return encode_clip(clips, length, fps)


def encode_clip(clips, length, fps):
    directory = bytearray()
    tracks = bytearray()
    table_end = 12 + 34 * len(clips)
    for samples in clips:
        start, end = samples[0][0], samples[-1][0] + 1
        directory.extend(struct.pack("<hh", start, end))
        for channel, fmt in enumerate(("<h3f", "<hf", "<h2f", "<hI", "<hh")):
            keys = [(t, sample[channel]) for t, sample in samples]
            if all(value == keys[0][1] for _, value in keys):
                keys = keys[:1]
            elif channel == 4:
                keys = [key for i, key in enumerate(keys) if i == 0 or key[1] != keys[i - 1][1]]
                # The engine holds the left key of a pair, including the final pair.
                keys.append((end, keys[-1][1]))
            if len(keys) > 32767:
                raise ValueError("A track exceeds the native limit of 32767 keys. Shorten the frame range.")
            directory.extend(struct.pack("<hi", len(keys), table_end + len(tracks)))
            for tick, value in keys:
                tracks.extend(struct.pack(fmt, tick, *value))
    return struct.pack("<iffi", table_end + len(tracks), length / fps, fps, len(clips)) + directory + tracks


class BK2_OT_particle_export(bpy.types.Operator, ExportHelper):
    bl_idname = "export_scene.bk2_particle"
    bl_label = "Export BK2 Particle Clip"
    filename_ext = ".ptc"
    filter_glob: StringProperty(default="*.ptc", options={"HIDDEN"})
    collection: StringProperty(name="Collection")
    unit_scale: FloatProperty(name="Game Units per Blender Unit", default=1, min=0.000001)

    def invoke(self, context, event):
        self.collection = context.collection.name
        return ExportHelper.invoke(self, context, event)

    def draw(self, context):
        self.layout.prop_search(self, "collection", bpy.data, "collections")
        self.layout.prop(self, "unit_scale")

    def execute(self, context):
        scene_root = context.scene.collection
        collection = scene_root if self.collection == scene_root.name else bpy.data.collections.get(self.collection)
        if collection is None:
            self.report({"ERROR"}, "Choose a collection in the current scene")
            return {"CANCELLED"}
        temporary = None
        try:
            data = bake(context, collection, self.unit_scale)
            destination = os.path.abspath(bpy.path.abspath(self.filepath))
            with tempfile.NamedTemporaryFile(dir=os.path.dirname(destination),
                                             suffix=".ptc.tmp", delete=False) as output:
                temporary = output.name
                output.write(data)
            os.replace(temporary, destination)
        except (OSError, ValueError, RuntimeError, OverflowError, struct.error) as error:
            self.report({"ERROR"}, str(error))
            return {"CANCELLED"}
        finally:
            if temporary and os.path.exists(temporary):
                os.unlink(temporary)
        self.report({"INFO"}, f"Exported {collection.name} to {os.path.basename(self.filepath)}")
        return {"FINISHED"}


def export_menu(self, context):
    self.layout.operator(BK2_OT_particle_export.bl_idname, text="Blitzkrieg 2 Particle Clip (.ptc)")


classes = (BK2ParticleAppearance, BK2_PT_emitter, BK2_OT_particle_export)


def register():
    for cls in classes:
        bpy.utils.register_class(cls)
    bpy.types.ParticleSettings.bk2_appearance = PointerProperty(type=BK2ParticleAppearance)
    bpy.types.TOPBAR_MT_file_export.append(export_menu)


def unregister():
    bpy.types.TOPBAR_MT_file_export.remove(export_menu)
    del bpy.types.ParticleSettings.bk2_appearance
    for cls in reversed(classes):
        bpy.utils.unregister_class(cls)
