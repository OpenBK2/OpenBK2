#!/usr/bin/env python3
"""Set Player to 0 for map objects with LinkID > 400 and 'fence' in Object href.

Writes a new UTF-8 XML file, defaulting to mapinfo.player0.xdb beside the input.
Existing output files are never overwritten. Requires only the Python standard
library. Use --dry-run to report matches without writing a file.
"""

import argparse
from pathlib import Path
import xml.etree.ElementTree as ET


def update_owners(tree):
    root = tree.getroot()
    if root.tag != 'MapInfo':
        raise ValueError('Expected a MapInfo XML root element')

    matched = 0
    changed = 0
    # Scope the search to placed objects, not player settings or other references.
    for item in root.findall('./Objects/Item'):
        reference = item.find('Object')
        if reference is None or 'fence' not in reference.get('href', '').casefold():
            continue
        # A missing link ID defaults to -1 in the game's map schema.
        if int(item.findtext('Link/LinkID', '-1')) <= 400:
            continue

        matched += 1
        player = item.find('Player')
        # An omitted Player already means player 0 in the game's map schema.
        if player is None or int(player.text or '0') == 0:
            continue
        player.text = '0'
        changed += 1

    return matched, changed


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('mapinfo', type=Path, help='Input mapinfo.xdb file')
    parser.add_argument('-o', '--output', type=Path, help='Destination file (must not exist)')
    parser.add_argument('--dry-run', action='store_true', help='Report changes without writing')
    args = parser.parse_args()
    output = args.output or args.mapinfo.with_name(args.mapinfo.stem + '.player0' + args.mapinfo.suffix)

    try:
        # Retain XML comments and processing instructions when serializing.
        xml_parser = ET.XMLParser(target=ET.TreeBuilder(insert_comments=True, insert_pis=True))
        tree = ET.parse(args.mapinfo, parser=xml_parser)
        matched, changed = update_owners(tree)
        if not args.dry_run:
            # Exclusive creation also protects the source if passed as --output.
            with output.open('xb') as stream:
                tree.write(stream, encoding='utf-8', xml_declaration=True)
                stream.write(b'\n')
    except (OSError, ET.ParseError, ValueError) as error:
        parser.exit(1, f'Error: {error}\n')

    action = 'Would change' if args.dry_run else 'Changed'
    print(f'{action} {changed} object(s) to player 0; {matched} matched the filters.')
    if not args.dry_run:
        print(f'Saved: {output}')


if __name__ == '__main__':
    main()
