#!/usr/bin/env python3
"""Check that types.xml and the generated DB sources still describe the same records.

Adding a field to the game database takes five coordinated edits: the <Item> in types.xml,
the C++ member, ReportMetaInfo(), operator&( IXmlSaver& ) and operator&( IBinSaver& ). The
sources carry an "automatically generated, don't change manually" banner but are edited by
hand, so it is easy to land four of the five. Nothing catches that at build time, because
each half compiles and loads perfectly well on its own; what breaks is the editor and the
runtime disagreeing about which chunk id a field lives in, or about the order of a field
list that is positional.

This reads types.xml and every struct that declares a `typeID`, and checks three things:

  * the ordered field names in types.xml match ReportMetaInfo() and operator&( IXmlSaver& )
  * the chunk ids in types.xml match operator&( IBinSaver& ), field for field
  * no two fields of one record share a chunk id

[noCode] fields are skipped: they exist for the editor and have no C++ member. Promoting a
field out of [noCode] therefore means giving it a fresh chunk id and moving it to where the
.cll declares it, which is exactly the mistake this catches.

    python scripts/check_db_schema.py            # baseline-aware, for CI
    python scripts/check_db_schema.py --strict   # report the known-stale records too
    python scripts/check_db_schema.py -v         # list every record, not just failures

Exits non-zero when a record outside the baseline disagrees.
"""

import argparse
import os
import re
import sys

# Records that already disagreed before this check existed. Each is a field present in the
# C++ and missing from types.xml, from a feature that landed without the types.xml edit.
# This is a record of existing drift, not approval of it: fix one and drop it from the list.
KNOWN_STALE = {
    'SAnimLight': 'uid (chunk 4) is serialized but has no types.xml <Item>',
    'SGameRoot': 'Fonts (chunk 8) is serialized but has no types.xml <Item>',
    'SReinforcement': 'TemplateOverride (chunk 9) is serialized but has no types.xml <Item>',
    'SWeatherDesc': 'PartMaterials (chunk 4) is serialized but has no types.xml <Item>',
}

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def top_level_items(body):
    """The top-level <Item> blocks of a <Fields> body.

    Some records are written one tag per line and others put a whole <Item> on a single
    line, and items nest inside <Attributes>, so track tag depth instead of reading lines.
    """
    out, depth, start = [], 0, None
    for m in re.finditer(r'</?Item>', body):
        if m.group(0) == '<Item>':
            if depth == 0:
                start = m.start()
            depth += 1
        else:
            depth -= 1
            if depth == 0:
                out.append(body[start:m.end()])
    return out


def read_schema(path):
    """{ TypeID: [(name, chunk id, is noCode)] } in declaration order."""
    data = open(path, encoding='utf-8').read()
    schema = {}
    for m in re.finditer(r'<TypeID>(\d+)</TypeID>', data):
        before = data[:m.start()].rstrip()
        if before.endswith('<Fields />'):
            schema[int(m.group(1))] = []
            continue
        if not before.endswith('</Fields>'):
            continue                    # a <TypeID> that is not closing a field list
        close = len(before) - len('</Fields>')
        body = data[data.rfind('<Fields>', 0, close) + len('<Fields>'):close]
        fields = []
        for blk in top_level_items(body):
            name = re.search(r'<Name>(.*?)</Name>', blk)
            chunk = re.search(r'<ChunkID>(\d+)</ChunkID>', blk)
            if name and chunk:
                fields.append((name.group(1), int(chunk.group(1)),
                               '<Key>noCode</Key>' in blk))
        schema[int(m.group(1))] = fields
    return schema


def read_sources(root):
    """({ struct: TypeID }, [source text]) for every record-bearing header and .cpp."""
    structs, sources = {}, []
    for dirpath, _, names in os.walk(root):
        for n in names:
            path = os.path.join(dirpath, n)
            if n.endswith('.h'):
                text = open(path, encoding='utf-8', errors='replace').read()
                for m in re.finditer(r'struct\s+(\w+)\s*(?::[^{]*)?\{(.*?)\n\t\};',
                                     text, re.S):
                    tid = re.search(r'enum\s*\{\s*typeID\s*=\s*(0x[0-9A-Fa-f]+|\d+)\s*\}',
                                    m.group(2))
                    if tid:
                        structs[m.group(1)] = int(tid.group(1), 0)
            elif n.endswith('.cpp'):
                sources.append(open(path, encoding='utf-8', errors='replace').read())
    return structs, sources


def function_body(text, signature):
    """The braced body of a function, by brace matching from its signature."""
    i = text.index('{', text.index(signature))
    depth, j = 0, i
    while True:
        if text[j] == '{':
            depth += 1
        elif text[j] == '}':
            depth -= 1
            if depth == 0:
                return text[i:j]
        j += 1


def read_record(text, struct):
    """(ReportMetaInfo names, IXmlSaver names, IBinSaver chunk ids) for one struct."""
    meta = [m.group(1) for m in re.finditer(
        r'NMetaInfo::Report\w*MetaInfo\(\s*"([^"]+)"',
        function_body(text, 'void %s::ReportMetaInfo() const' % struct))]
    xml = [m.group(1) for m in re.finditer(
        r'saver\.Add\(\s*"([^"]+)"',
        function_body(text, 'int %s::operator&( IXmlSaver &saver )' % struct))]
    chunks = []
    for m in re.finditer(
            r'(?:saver\.Add|AddUuidChunk\(\s*saver,)\(?\s*(\d+)\s*,\s*(.*?)\s*\)\s*;',
            function_body(text, 'int %s::operator&( IBinSaver &saver )' % struct)):
        if 'this' in m.group(2):
            continue                    # the base-class chunk, not a field of this record
        chunks.append(int(m.group(1)))
    return meta, xml, chunks


def check(schema, structs, sources):
    """[(struct, [complaint])] for every record whose halves disagree."""
    failures = []
    checked = 0
    for struct, tid in sorted(structs.items()):
        if tid not in schema:
            continue
        signature = 'void %s::ReportMetaInfo() const' % struct
        text = next((t for t in sources
                     if signature in t
                     and 'int %s::operator&( IBinSaver &saver )' % struct in t
                     and 'int %s::operator&( IXmlSaver &saver )' % struct in t), None)
        if text is None:
            continue
        checked += 1
        meta, xml, chunks = read_record(text, struct)
        coded = [(n, c) for n, c, no_code in schema[tid] if not no_code]
        names = [n for n, _ in coded]
        ids = [c for _, c in coded]

        complaints = []
        if names != meta:
            complaints.append('types.xml order %s\n      ReportMetaInfo   %s' % (names, meta))
        if names != xml:
            complaints.append('types.xml order %s\n      IXmlSaver        %s' % (names, xml))
        if ids != chunks:
            complaints.append('types.xml chunks %s\n      IBinSaver        %s' % (ids, chunks))
        duplicates = sorted({c for c in ids if ids.count(c) > 1})
        if duplicates:
            complaints.append('types.xml reuses chunk id(s) %s among coded fields' % duplicates)
        if complaints:
            failures.append((struct, complaints))
    return checked, failures


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--types', default=os.path.join(REPO, 'types.xml'))
    parser.add_argument('--root',
                        default=os.path.join(REPO, 'Versions', 'Temporary', 'Engine', 'Sources'))
    parser.add_argument('--strict', action='store_true',
                        help='report the known-stale records too')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='list every record that was checked')
    args = parser.parse_args()

    schema = read_schema(args.types)
    structs, sources = read_sources(args.root)
    checked, failures = check(schema, structs, sources)

    failed = [(s, c) for s, c in failures if args.strict or s not in KNOWN_STALE]
    baselined = [s for s, _ in failures if not args.strict and s in KNOWN_STALE]

    if args.verbose:
        broken = {s for s, _ in failures}
        for struct in sorted(structs):
            if structs[struct] in schema:
                print('  %-40s %s' % (struct, 'MISMATCH' if struct in broken else 'ok'))

    for struct, complaints in failed:
        print('%s:' % struct)
        for c in complaints:
            print('      %s' % c)

    print('%d records checked, %d agree, %d disagree'
          % (checked, checked - len(failures), len(failures)))
    if baselined:
        print('known stale, not failing the run (use --strict to see them): %s'
              % ', '.join(sorted(baselined)))

    fixed = sorted(set(KNOWN_STALE) - {s for s, _ in failures})
    if fixed:
        print('no longer stale, drop from KNOWN_STALE in this script: %s' % ', '.join(fixed))

    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
