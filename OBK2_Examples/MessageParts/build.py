"""Build the English message archive from the editable UTF-8 catalogue."""

import json
from pathlib import Path
import re
from xml.etree import ElementTree as ET
import zipfile


def build():
    directory = Path(__file__).resolve().parent
    messages = json.loads((directory / "messages.json").read_text(encoding="utf-8"))
    root = ET.Element("MessageParts")
    parts = ET.SubElement(root, "Parts")
    entries = {}
    prefix = "Other/Text/Game/Messages/"
    for name, text in messages.items():
        if not re.fullmatch(r"[A-Z][A-Z0-9_]*", name):
            raise ValueError(f"Invalid ValName: {name}")
        item = ET.SubElement(parts, "Item")
        ET.SubElement(item, "ValName").text = name
        ET.SubElement(item, "ValueTextFileRef", href=name + ".txt")
        # NText reads UTF-16LE with a BOM. Preserve prefix/suffix spaces exactly;
        # a final newline would become part of the displayed message.
        entries[prefix + name + ".txt"] = b"\xff\xfe" + text.encode("utf-16le")
    ET.indent(root, space="\t")
    entries[prefix + "MessageParts.xdb"] = (
        ET.tostring(root, encoding="utf-8", xml_declaration=True) + b"\n"
    )
    # Keep the repository's linked game data identical to the distributable pack.
    data_root = directory.parents[1] / "Versions/Current/Data"
    for name, content in entries.items():
        target = data_root / name
        target.parent.mkdir(parents=True, exist_ok=True)
        if not target.exists() or target.read_bytes() != content:
            target.write_bytes(content)
    with zipfile.ZipFile(directory / "messages.pak", "w") as archive:
        for name, content in sorted(entries.items()):
            # Fixed metadata keeps rebuilding the same catalogue byte-identical.
            info = zipfile.ZipInfo(name, date_time=(2000, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.create_system = 3
            info.external_attr = 0o100644 << 16
            archive.writestr(info, content)


if __name__ == "__main__":
    build()
