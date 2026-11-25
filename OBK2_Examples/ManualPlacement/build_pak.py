"""Build the game room's optional placement controls with retail artwork."""

from pathlib import Path
from struct import unpack_from
import xml.etree.ElementTree as ET
from zipfile import ZipFile, ZipInfo, ZIP_DEFLATED, ZIP_STORED


BASE = "UI/Game/Menu/MPGameRoom/Placement/"
MENU = "/UI/Game/Menu/"
TEAM = MENU + "MPStatistics/StatsList/StatsItem/TeamColour"
DICE = MENU + "MPGameRoom/Countries/FlagRandom_Texture"
GAME_DATA = Path(__file__).resolve().parents[2] / "Versions/Current/Data"
GREY = "/Scene/TexAndMats/All/UI/Mission/Minimap/MP/MinimapFlagGrey_Texture"
WINDOW_CLASSES = {"WindowSimple": 285700801, "WindowMSButton": 285694855, "WindowTextView": 285694860}


def node(parent, tag, value):
    ET.SubElement(parent, tag).text = str(value)


def ref(parent, tag, path, kind):
    ET.SubElement(parent, tag, href=f"{path}.xdb#xpointer(/{kind})")


def window(kind, shared, name, x, y, width, height, priority=0, visible=True):
    root = ET.Element(kind)
    node(root, "ClassTypeID", WINDOW_CLASSES[kind])
    ref(root, "Shared", shared, kind + "Shared")
    node(root, "Name", name)
    node(root, "Priority", priority)
    node(root, "Visible", "true" if visible else "false")
    node(root, "Enabled", "true")
    placement = ET.SubElement(root, "Placement")
    for field, a, b in (("Position", x, y), ("Size", width, height)):
        pair = ET.SubElement(placement, field)
        first = ET.SubElement(pair, "First")
        node(first, "x", a)
        node(first, "y", b)
        node(pair, "Second", "true")
    for field in ("HorAllign", "VerAllign"):
        pair = ET.SubElement(placement, field)
        node(pair, "First", "EPA_LOW_END")
        node(pair, "Second", "true")
    return root


def shared(kind, transparent=True):
    root = ET.Element(kind + "Shared")
    node(ET.SubElement(root, "Flags"), "Transparent", "true" if transparent else "false")
    return root


def background(texture, tint=-1):
    root = ET.Element("BackgroundSimpleScallingTexture")
    node(root, "ClassTypeID", 0x11075B40)
    if texture:
        ref(root, "Texture", texture, "Texture")
    node(root, "Color", tint)
    return root


def rounded_dice():
    # The retail dice is a single uncompressed BGRA mip. Preserve its colours
    # and apply an antialiased three-texel radius only to the lobby button.
    path = GAME_DATA / (DICE.lstrip("/") + ".dds")
    dds = bytearray(path.read_bytes())
    height, width = unpack_from("<II", dds, 12)
    if (dds[:4] != b"DDS " or unpack_from("<III", dds, 80) != (0x41, 0, 32)
            or unpack_from("<I", dds, 104)[0] != 0xff000000 or len(dds) != 128 + width * height * 4):
        raise ValueError(f"Expected a single 32-bit BGRA texture in {path}")
    radius, samples = 3, 8
    for y in range(radius):
        for x in range(radius):
            coverage = sum(
                (x + (sx + 0.5) / samples - radius) ** 2 + (y + (sy + 0.5) / samples - radius) ** 2 <= radius ** 2
                for sy in range(samples) for sx in range(samples)
            )
            for corner_y in (y, height - 1 - y):
                for corner_x in (x, width - 1 - x):
                    alpha = 128 + 4 * (width * corner_y + corner_x) + 3
                    dds[alpha] = round(dds[alpha] * coverage / (samples * samples))
    return bytes(dds)


def build():
    files = {}

    def xml(name, root):
        ET.indent(root, space="\t")
        files[BASE + name + ".xdb"] = ET.tostring(root, encoding="utf-8", xml_declaration=True) + b"\n"

    texture = ET.parse(GAME_DATA / (DICE.lstrip("/") + ".xdb")).getroot()
    texture.attrib.clear()
    texture.find("DestName").set("href", "DiceRounded.dds")
    xml("DiceRounded_Texture", texture)
    files[BASE + "DiceRounded.dds"] = rounded_dice()
    xml("Dice", background("DiceRounded_Texture"))
    xml("DiceDark", background("DiceRounded_Texture", -8355712))  # opaque 0xff808080
    # The black interior starts at x=19 and ends at y=260 in MinimapBack_Texture.
    # The room adds equal screen-space margins to this flush lower-left position.
    button = window("WindowMSButton", "Toggle_Shared", "ManualPlacement", 19, 238, 32, 22, 10)
    ET.SubElement(button, "TooltipFileRef", href="Toggle.txt")
    button_shared = shared("WindowMSButton", transparent=False)
    node(button_shared, "TriggerMode", "BCST_ON_RELEASE")
    logical_states = ET.SubElement(button, "ButtonStates")
    visual_states = ET.SubElement(button_shared, "VisualStates")
    for appearance in ("Dice", "DiceDark"):
        logical = ET.SubElement(logical_states, "Item")
        commands = ET.SubElement(ET.SubElement(logical, "CommandsOnEnterState"), "Commands")
        ref(commands, "Item", "Toggle_Reaction", "UISRunReaction")
        visual = ET.SubElement(visual_states, "Item")
        for substate in ("Normal", "MouseOver", "Pushed", "Disabled"):
            ref(ET.SubElement(visual, substate), "Background",
                "DiceDark" if substate in ("Pushed", "Disabled") else appearance, "BackgroundSimpleScallingTexture")
    xml("Toggle_WindowMSButton", button)
    xml("Toggle_Shared", button_shared)
    reaction = ET.Element("UISRunReaction")
    pair = ET.SubElement(reaction, "szReactionForward")
    node(pair, "First", "ToggleManualPlacement")
    node(pair, "Second", "true")
    xml("Toggle_Reaction", reaction)
    tooltip = ("Click to roll starting positions for everyone. Drag one flag onto another to swap. "
               "Click again to restore the room's placement setting.")
    files[BASE + "Toggle.txt"] = b"\xff\xfe" + tooltip.encode("utf-16-le")

    # Scale the loading screen's team overlays to the smaller lobby map. Their
    # transparent pixels reveal the same player-colour block used during loading.
    flag = shared("WindowSimple")
    children = ET.SubElement(flag, "Children")
    for name, texture, x, y, w, h in (
        ("TeamBlock", None, 0, 0, 32, 19),
        ("Team1", TEAM + "Green_Texture", 0, 0, 32, 19),
        ("Team2", TEAM + "Red_Texture", 0, 0, 32, 19),
        ("Empty", GREY, 8, 3, 16, 13),
    ):
        ref(children, "Item", name, "WindowSimple")
        xml(name, window("WindowSimple", name + "_Shared", name, x, y, w, h, 1 if texture else 0))
        layer = shared("WindowSimple")
        if texture:
            xml(name + "_Art", background(texture))
            ref(layer, "Background", name + "_Art", "BackgroundSimpleScallingTexture")
        xml(name + "_Shared", layer)

    # The loading screen places its nation icon over the upper-left of the team
    # pennant. Leave room for the team shape and player colour around the lobby flag.
    ref(children, "Item", "Icon", "WindowSimple")
    xml("Icon", window("WindowSimple", "Icon_Shared", "Icon", 1, 1, 16, 11, 2, visible=False))
    icon_shared = shared("WindowSimple")
    ref(icon_shared, "Background", "Icon_Art", "BackgroundSimpleScallingTexture")
    xml("Icon_Shared", icon_shared)
    xml("Icon_Art", background(None))
    xml("Flag_Shared", flag)
    xml("Flag_WindowSimple", window("WindowSimple", "Flag_Shared", "PlacementFlag", 0, 0, 32, 19, 3))
    xml("Name_Shared", shared("WindowTextView"))
    label = window("WindowTextView", "Name_Shared", "PlacementName", 0, 0, 120, 20, 4)
    ref(label, "TextFormat", MENU + "MPLoading2/Player_TextFormat", "TextFormat")
    xml("Name_WindowTextView", label)

    with ZipFile(Path(__file__).with_name("manual_placement.pak"), "w") as archive:
        for name, data in sorted(files.items()):
            entry = ZipInfo(name, (2026, 1, 1, 0, 0, 0))
            entry.create_system = 3
            entry.external_attr = 0o100644 << 16
            archive.writestr(entry, data, ZIP_STORED if name.endswith(".txt") else ZIP_DEFLATED)


if __name__ == "__main__":
    build()
