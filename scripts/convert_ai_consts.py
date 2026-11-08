#!/usr/bin/env python3
"""Convert AIConsts.cpp profile settings into an AIPropertyConsts resource.

Unspecified properties use the schema's runtime defaults, not the console
registry's sometimes different getvar/reset defaults. Other profile settings
are outside this migration. This reads one file; it does not execute scripts.
"""

import argparse
from decimal import Decimal
from pathlib import Path
import re
import xml.etree.ElementTree as ET


ROOT = Path(__file__).resolve().parents[1]
SCHEMA = ROOT / 'Versions/Temporary/Engine/Sources/AILogic/DBAIConsts.cll'


def format_value(value, kind):
    value = value.strip().strip('"')
    if kind == 'bool':
        if value.lower() in ('true', 'false'):
            return value.lower()
        return 'true' if Decimal(value) != 0 else 'false'
    number = Decimal(value)
    if not number.is_finite():
        raise ValueError(f'Non-finite property value: {value}')
    if kind == 'int':
        return str(int(number))
    return str(number)


def convert(profile, output):
    source = profile.read_text(encoding='utf-8-sig')
    source = re.sub(r'//[^\n]*', '', source)
    settings = dict(re.findall(r'^\s*setvar\s+(\S+)\s*=\s*([^\r\n]+)', source, re.M))
    schema = SCHEMA.read_text(encoding='utf-8')
    body = re.search(r'class SAIPropertyConsts\s*\{(.*?)\n\};', schema, re.S).group(1)
    fields = re.findall(
        r'// Legacy setting: ([^;]+);[^\n]*\n\s*(float|int|bool) _([\w]+) = ([^;]+);', body)
    if not fields:
        raise ValueError('No AI property bindings found in the schema')

    resource = ET.Element('AIPropertyConsts')
    configured = 0
    for key, kind, name, default in fields:
        ET.SubElement(resource, name).text = format_value(settings.get(key, default), kind)
        configured += key in settings

    reveal = {}
    for key, value in settings.items():
        match = re.fullmatch(r'AI\.RevealInfo\.([^.]+)\.(Query|MovingOff|Distance|Time)', key)
        if match:
            reveal.setdefault(match[1], {})[match[2]] = value
    table = ET.SubElement(resource, 'RevealInfo')
    for unit_type, values in sorted(reveal.items()):
        item = ET.SubElement(table, 'Item')
        ET.SubElement(item, 'UnitType').text = unit_type
        for name in ('Query', 'MovingOff', 'Distance', 'Time'):
            kind = 'int' if name == 'Time' else 'float'
            ET.SubElement(item, name).text = format_value(values.get(name, '0'), kind)

    ET.indent(resource, space='\t')
    output.parent.mkdir(parents=True, exist_ok=True)
    contents = '<?xml version="1.0" encoding="UTF-8"?>\n' + ET.tostring(resource, encoding='unicode') + '\n'
    with output.open('w', encoding='utf-8', newline='\n') as stream:
        stream.write(contents)
    print(f'{len(fields)} properties: {configured} from profile, '
          f'{len(fields) - configured} from runtime defaults; {len(reveal)} reveal entries')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('profile', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    convert(args.profile, args.output)


if __name__ == '__main__':
    main()
