#!/usr/bin/env python3
"""Build the multiplayer unit transfer HUD resources and deterministic ZIP .pak.

Only new database paths are added, so existing mission screen mods remain intact.
All artwork and state colors come from the game's existing UI resources.
"""

import argparse
from pathlib import Path
import xml.etree.ElementTree as ET
import zipfile


HERE = Path(__file__).resolve().parent
BASE = Path("UI/Game/Mission/UnitTransfer")
GAME_DATA = HERE.parents[1] / "Versions/Current/Data"
# Preserve the stock normal, pressed and disabled appearances. Only their geometry
# is adapted to these small controls; no palette or texture artwork is introduced.
NATIVE_BACKGROUNDS = {
    "ToggleNormal": "UI/Game/Menu/Common/Button_BackgroundTiledTexture.xdb",
    "TogglePushed": "UI/Game/Menu/Common/Button_Pressed_BackgroundTiledTexture.xdb",
    "ToggleDisabled": "UI/Game/Menu/Common/Button_Forbidden_BackgroundTiledTexture.xdb",
    "ConfirmNormal": "UI/Game/Mission/NewActions/TemplateNormal_BackgroundSimpleTexture.xdb",
    "ConfirmPushed": "UI/Game/Mission/NewActions/TemplatePressed_BackgroundSimpleTexture.xdb",
    "ConfirmDisabled": "UI/Game/Mission/NewActions/TemplateDisabled_BackgroundSimpleTexture.xdb",
}
CLASS_IDS = {
    "WindowSimple": 285700801,
    "WindowMSButton": 285694855,
    "WindowTextView": 285694860,
    "WindowScrollableContainer": 386593537,
    "WindowScrollBar": 285694857,
    "WindowSlider": 285694858,
}


def child(parent, tag, text=None, **attrs):
    result = ET.SubElement(parent, tag, attrs)
    if text is not None:
        result.text = str(text).lower() if isinstance(text, bool) else str(text)
    return result


def reference(parent, tag, name, kind=None):
    href = name + (f"#xpointer(/{kind})" if kind else "")
    return child(parent, tag, href=href)


def placement(parent, x, y, width, height, horizontal="EPA_LOW_END", vertical="EPA_LOW_END", margins=(0, 0, 0, 0)):
    result = child(parent, "Placement")
    for tag, values in (("Position", (x, y)), ("Size", (width, height)),
                        ("LowerMargin", margins[:2]), ("UpperMargin", margins[2:])):
        pair = child(result, tag)
        first = child(pair, "First")
        child(first, "x", values[0])
        child(first, "y", values[1])
        child(pair, "Second", True)
    for tag, value in (("HorAllign", horizontal), ("VerAllign", vertical)):
        pair = child(result, tag)
        child(pair, "First", value)
        child(pair, "Second", True)


def window(kind, shared, name, x, y, width, height, **kwargs):
    result = ET.Element(kind)
    child(result, "ClassTypeID", CLASS_IDS[kind])
    reference(result, "Shared", shared + ".xdb", kind + "Shared")
    child(result, "Name", name)
    child(result, "Visible", True)
    child(result, "Enabled", True)
    child(result, "Priority", 0)
    child(result, "GameMessageReactions")
    placement(result, x, y, width, height, **kwargs)
    return result


def shared(kind, background="", children=(), transparent=False, background_kind="BackgroundTiledTexture"):
    result = ET.Element(kind + "Shared")
    child(result, "Crap", False)
    items = child(result, "Children")
    for name, item_kind in children:
        reference(items, "Item", name + ".xdb", item_kind)
    reference(result, "Background", background + ".xdb" if background else "",
              background_kind if background else None)
    reference(result, "Foreground", "")
    child(child(result, "Flags"), "Transparent", transparent)
    child(result, "ActiveArea")
    child(result, "IgnoreDblClick", False)
    return result


def build():
    entries = {}

    def xml(name, element):
        ET.indent(element, space="\t")
        entries[str(BASE / (name + ".xdb")).replace("\\", "/")] = (
            ET.tostring(element, encoding="utf-8", xml_declaration=True) + b"\n"
        )

    def text(name, value):
        # A trailing newline creates another measured text line and offsets vertical centering.
        entries[str(BASE / (name + ".txt")).replace("\\", "/")] = b"\xff\xfe" + value.encode("utf-16-le")

    for name, source in NATIVE_BACKGROUNDS.items():
        background = ET.parse(GAME_DATA / source).getroot()
        background.attrib.clear()
        texture = background.find("Texture")
        texture_path = texture.get("href").split("#")[0]
        texture.set("href", "/" + (Path(source).parent / texture_path).as_posix() + "#xpointer(/Texture)")
        if background.tag == "BackgroundTiledTexture":
            # Stock menu buttons are 42px high; keep their complete border inside 34px.
            for strip in ("rL", "rR", "rF"):
                background.find(strip + "/Size/y").text = "34"
        else:
            background.tag = "BackgroundSimpleScallingTexture"
            background.find("ClassTypeID").text = "0"
            background.remove(background.find("TextureX"))
            background.remove(background.find("TextureY"))
            size = child(background, "Size")
            child(size, "x", 0)
            child(size, "y", 0)
        xml(name + "_Background", background)

    # Use the native artwork for checkmarks and the toggle's open/closed direction.
    for name, texture in (
        ("Check", "/UI/Game/Menu/MultiScreen/IconChecked_Texture"),
        ("TransferArrowRight", "/UI/Game/Mission/Panels/RightPanel/ToggleReinfRightNormal_Texture"),
        ("TransferArrowLeft", "/UI/Game/Mission/Panels/RightPanel/ToggleReinfLeftNormal_Texture"),
    ):
        background = ET.Element("BackgroundSimpleScallingTexture")
        child(background, "ClassTypeID", 0)
        reference(background, "Texture", texture + ".xdb", "Texture")
        child(background, "Color", -1)
        child(child(background, "Size"), "x", 0)
        child(background.find("Size"), "y", 0)
        xml(name + "_Background", background)
        if name != "Check":
            xml(name + "_WindowSimpleShared", shared("WindowSimple", name + "_Background", transparent=True,
                                                     background_kind="BackgroundSimpleScallingTexture"))
            icon = window("WindowSimple", name + "_WindowSimpleShared", name, 84, 5, 24, 24)
            icon.find("Visible").text = "false" if name.endswith("Left") else "true"
            xml(name + "_WindowSimple", icon)

    xml("Text_WindowTextViewShared", shared("WindowTextView", transparent=True))
    xml("Empty_WindowSimpleShared", shared("WindowSimple"))

    def label(name, caption, x, y, width, height, center=False, vertical_center=False):
        # Auto-height plus centered placement follows the actual font metrics, including HUD scale changes.
        item = window("WindowTextView", "Text_WindowTextViewShared", name, x, y, width, height,
                      vertical="ERA_CENTER" if vertical_center else "EPA_LOW_END")
        child(item, "ResizeOnTextSet", vertical_center)
        reference(item, "TextFormat", "/UI/Game/Common/Normal" + ("_center" if center else "") + "_TextFormat.xdb", "TextFormat")
        if caption:
            text(name, caption)
            reference(item, "TextFileRef", name + ".txt")
        xml(name + "_WindowTextView", item)

    def button(name, reaction, x, y, width, height, appearance, check=False, caption=None, extra_children=()):
        children = list(extra_children)
        if caption:
            label(name + "Label", caption, 2, 0, 80, height, center=True, vertical_center=True)
            children.append((name + "Label_WindowTextView", "WindowTextView"))
        desc = shared("WindowMSButton", children=children)
        state = child(child(desc, "VisualStates"), "Item")
        background_kind = "BackgroundTiledTexture" if appearance == "Toggle" else "BackgroundSimpleScallingTexture"
        # Native menu buttons retain their normal texture on hover.
        for substate, suffix in (("Normal", "Normal"), ("MouseOver", "Normal"), ("Pushed", "Pushed"), ("Disabled", "Disabled")):
            visual = child(state, substate)
            reference(visual, "Background", appearance + suffix + "_Background.xdb", background_kind)
            if check and substate != "Disabled":
                reference(visual, "Foreground", "Check_Background.xdb", "BackgroundSimpleScallingTexture")
        child(state, "DefaultSubState", "BST_NORMAL")
        child(desc, "TriggerMode", "BCST_ON_RELEASE")
        xml(name + "_WindowMSButtonShared", desc)

        desc = window("WindowMSButton", name + "_WindowMSButtonShared", name, x, y, width, height)
        state = child(child(desc, "ButtonStates"), "Item")
        animation = child(state, "CommandsOnEnterState")
        reference(child(animation, "Commands"), "Item", reaction + "_UISRunReaction.xdb", "UISRunReaction")
        child(animation, "Reversable", False)
        child(state, "WaitVisual", False)
        child(state, "ReverseCommands", False)
        xml(name + "_WindowMSButton", desc)

        desc = ET.Element("UISRunReaction")
        child(desc, "ClassTypeID", 0)
        pair = child(desc, "szReactionForward")
        child(pair, "First", reaction)
        child(pair, "Second", True)
        xml(reaction + "_UISRunReaction", desc)

    button("Toggle", "MissionTransferToggle", 0, 0, 110, 34, "Toggle", caption="Transfer", extra_children=(
        ("TransferArrowRight_WindowSimple", "WindowSimple"),
        ("TransferArrowLeft_WindowSimple", "WindowSimple"),
    ))
    button("TransferConfirm", "MissionTransferPlayer", 154, 1, 24, 24, "Confirm", check=True)
    label("TransferName", None, 5, 0, 146, 26, vertical_center=True)
    label("TransferTitle", "Transfer units", 8, 5, 166, 24)

    xml("Row_WindowSimpleShared", shared("WindowSimple", "/UI/Game/Menu/EscapeMenu/HeaderTable_BackgroundTiledTexture", (
        ("TransferName_WindowTextView", "WindowTextView"),
        ("TransferConfirm_WindowMSButton", "WindowMSButton"),
    )))
    xml("Row_WindowSimple", window("WindowSimple", "Row_WindowSimpleShared", "TransferRow", 0, 0, 180, 26))

    # Keep the clipped viewport separate from the scrollbar and its arrows.
    xml("Viewport_WindowSimple", window("WindowSimple", "Empty_WindowSimpleShared", "", 0, 0, 180, 150,
                                         horizontal="EPA_MARGIN", vertical="EPA_MARGIN", margins=(0, 0, 18, 0)))
    # The native artwork is 16px wide even though stock button descriptors reserve 32px.
    # Keep the middle track visible when the slider hides its thumb for a short list.
    desc = shared("WindowSlider", "/UI/Game/Menu/EscapeMenu/EscMenu_Slider_BackgroundTiledTexture")
    child(desc, "Horisontal", False)
    reference(desc, "Lever", "/UI/Game/Menu/Common/ScrollBar/ScrollSlider_WindowMSButton.xdb", "WindowMSButton")
    child(desc, "MinLeverSize", 26)
    child(desc, "MaxLeverSize", 0)
    xml("Players_WindowSliderShared", desc)
    slider = window("WindowSlider", "Players_WindowSliderShared", "", 0, 0, 16, 0,
                    vertical="EPA_MARGIN", margins=(0, 32, 0, 32))
    child(slider, "SpecialPositions", 0)
    xml("Players_WindowSlider", slider)
    for direction, alignment in (("Up", "EPA_LOW_END"), ("Down", "EPA_HIGH_END")):
        arrow = window("WindowMSButton", "/UI/Game/Menu/Common/ScrollBar/Scroll" + direction + "_WindowMSButtonShared",
                       "", 0, 0, 16, 32, vertical=alignment)
        child(child(arrow, "ButtonStates"), "Item")
        xml("PlayersScroll" + direction + "_WindowMSButton", arrow)
    desc = shared("WindowScrollBar")
    child(desc, "Speed", 200)
    reference(desc, "ButtonLower", "PlayersScrollUp_WindowMSButton.xdb", "WindowMSButton")
    reference(desc, "ButtonGreater", "PlayersScrollDown_WindowMSButton.xdb", "WindowMSButton")
    reference(desc, "Slider", "Players_WindowSlider.xdb", "WindowSlider")
    xml("Players_WindowScrollBarShared", desc)
    scrollbar = window("WindowScrollBar", "Players_WindowScrollBarShared", "", 0, 0, 16, 0,
                       horizontal="EPA_HIGH_END", vertical="EPA_MARGIN", margins=(0, 0, 0, 0))
    xml("Players_WindowScrollBar", scrollbar)
    desc = shared("WindowScrollableContainer")
    child(desc, "Interval", 2)
    reference(desc, "ScrollBar", "Players_WindowScrollBar.xdb", "WindowScrollBar")
    reference(desc, "Border", "Viewport_WindowSimple.xdb", "WindowSimple")
    xml("Players_WindowScrollableContainerShared", desc)
    xml("Players_WindowScrollableContainer", window("WindowScrollableContainer", "Players_WindowScrollableContainerShared",
                                                     "TransferPlayers", 5, 33, 198, 150))

    xml("Panel_WindowSimpleShared", shared("WindowSimple", "/UI/Game/Menu/EscapeMenu/EscMenu_Bgr_4_BackgroundTiledTexture", (
        ("TransferTitle_WindowTextView", "WindowTextView"),
        ("Players_WindowScrollableContainer", "WindowScrollableContainer"),
    )))
    xml("Panel_WindowSimple", window("WindowSimple", "Panel_WindowSimpleShared", "UnitTransferPanel", 0, 0, 208, 188))

    # The popover draws above the mission HUD; modal screens retain their own priority.
    for name in ("Toggle_WindowMSButton", "Panel_WindowSimple"):
        key = (BASE / (name + ".xdb")).as_posix()
        desc = ET.fromstring(entries[key])
        desc.find("Visible").text = "false"
        desc.find("Priority").text = "10"
        xml(name, desc)

    return entries


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=HERE / "unit_transfer.pak")
    parser.add_argument("--check", action="store_true", help="verify the archive matches its generated resources")
    args = parser.parse_args()
    entries = build()
    if args.check:
        with zipfile.ZipFile(args.output) as archive:
            actual = {name: archive.read(name) for name in archive.namelist()}
        if actual != entries:
            raise SystemExit("unit_transfer.pak differs from the generated resources")
        print(f"Verified {len(entries)} resources in {args.output.name}")
        return
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(args.output, "w", compression=zipfile.ZIP_DEFLATED) as archive:
        for name, data in sorted(entries.items()):
            entry = zipfile.ZipInfo(name, date_time=(2026, 1, 1, 0, 0, 0))
            # Short localized labels can grow under DEFLATE, so store them verbatim.
            entry.compress_type = zipfile.ZIP_STORED if name.endswith(".txt") else zipfile.ZIP_DEFLATED
            entry.external_attr = 0o644 << 16
            archive.writestr(entry, data)
    print(f"Wrote {len(entries)} resources to {args.output}")


if __name__ == "__main__":
    main()
