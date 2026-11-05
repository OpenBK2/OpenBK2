#!/usr/bin/env python3
"""Repair an earlier editor icon export without rendering its DDS images again."""

import argparse
from pathlib import Path
import re
import xml.etree.ElementTree as ET
import zipfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="Existing MechUnitIcons.pak")
    parser.add_argument("--output", type=Path,
                        default=Path(__file__).with_name("MechUnitIcons.pak"))
    args = parser.parse_args()
    if args.source.resolve() == args.output.resolve():
        parser.error("Use a different output path to preserve the original archive.")

    count = 0
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(args.source) as source, zipfile.ZipFile(args.output, "w") as output:
        for entry in source.infolist():
            data = source.read(entry)
            if entry.filename.startswith("MechUnitIcons/") and entry.filename.endswith(".icon.xdb"):
                if ET.fromstring(data).tag != "Texture":
                    raise ValueError(f"Unexpected texture record: {entry.filename}")
                # Change the logical UI footprint only. Preserve the XML, unit
                # records, DDS bytes, UTF-8 names and timestamps otherwise.
                for field in (b"Width", b"Height"):
                    data, replacements = re.subn(
                        rb"<" + field + rb">\s*\d+\s*</" + field + rb">",
                        b"<" + field + b">42</" + field + b">", data)
                    if replacements != 1:
                        raise ValueError(f"Missing or duplicate dimensions: {entry.filename}")
                count += 1
            output.writestr(entry, data)
    with zipfile.ZipFile(args.output) as result:
        bad_entry = result.testzip()
        if bad_entry is not None:
            raise ValueError(f"Archive checksum failed: {bad_entry}")
    print(f"Corrected {count} texture records: {args.output}")


if __name__ == "__main__":
    main()
