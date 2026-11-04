"""Build the multiplayer difficulty UI archive from its editable loose sources."""

import json
from pathlib import Path, PurePosixPath
from xml.etree import ElementTree as ET
import zipfile


def build():
    directory = Path(__file__).resolve().parent
    data_root = directory / "Data"
    strings = json.loads((directory / "strings.json").read_text(encoding="utf-8"))
    for name, text in strings.items():
        relative = PurePosixPath(name)
        if relative.is_absolute() or ".." in relative.parts or relative.suffix != ".txt":
            raise ValueError(f"Invalid game text path: {name}")
        target = data_root / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        # NText requires UTF-16LE with a BOM; a final newline is displayed literally.
        content = b"\xff\xfe" + text.encode("utf-16le")
        if not target.exists() or target.read_bytes() != content:
            target.write_bytes(content)

    entries = {}
    for path in sorted(data_root.rglob("*")):
        if not path.is_file():
            continue
        if path.suffix == ".xdb":
            ET.parse(path)
        entries[path.relative_to(data_root).as_posix()] = path.read_bytes()

    with zipfile.ZipFile(directory / "MultiplayerDifficulty.pak", "w") as archive:
        for name, content in entries.items():
            # Stable metadata makes rebuilding unchanged sources byte-identical.
            # VFS picks the newest member for duplicate paths, so this must postdate stock packs.
            info = zipfile.ZipInfo(name, date_time=(2026, 11, 4, 12, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.create_system = 3
            info.external_attr = 0o100644 << 16
            archive.writestr(info, content)
    print(f"Packed {len(entries)} files into MultiplayerDifficulty.pak")


if __name__ == "__main__":
    build()
