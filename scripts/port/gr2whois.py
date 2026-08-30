#!/usr/bin/env python3
"""Turn a file hash out of a libgr2 log back into the unit it belongs to.

libgr2 and the recording shim name every file they load by the FNV-1a hash of
its bytes, because that is the only identity the engine gives them: a resource
comes out of a pak through the VFS and arrives as a naked buffer. A log line
reads

    file#7 6599883324a5ec0b "J:/Complete/Units/Infantry/Animations/RIFLE/2.mb"
    6645 bytes: skeletons=1 bones=[21] models=1 ...

and the exporter's own name is the same for every RIFLE resource in the game, so
the hash is what tells them apart. This script closes that loop:

    hash -> the pak entry, whose name is the resource GUID
         -> the .xdb record carrying that GUID
         -> the .xdb records that reference it, up as many levels as asked

which for the example above ends at the Soviet rifleman.

    python gr2whois.py 6599883324a5ec0b
    python gr2whois.py --log granny_calls.log          every file a run loaded
    python gr2whois.py --log granny_calls.log --warned only the ones a warning named
    python gr2whois.py 6599883324a5ec0b --depth 3

The first run builds an index over the installed games, which takes a couple of
minutes; after that it is cached beside this script and refreshed only when a
pak changes. Use --roots to point it somewhere else, or --refresh to rebuild.
"""
import argparse
import json
import os
import posixpath
import re
import sys
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import gr2diff

#! Beside the script, in a dot directory .gitignore covers.
#!
#! It is tens of megabytes and derived entirely from somebody's game install, so
#! it is neither worth committing nor portable between machines.
CACHE_PATH = os.path.join(HERE, '.gr2cache', 'whois-index.json')

#! Where GR2 resources live inside a pak.
GR2_DIRECTORIES = ('bin/geometries/', 'bin/animations/', 'bin/skeletons/',
                   'bin/aigeometries/')

UID_RE = re.compile(rb'<uid>\s*([0-9A-Fa-f-]{36})\s*</uid>')
HREF_RE = re.compile(rb'href\s*=\s*"([^"]+)"')


def fnv1a64(data):
    """The hash Identify.cpp computes. The two have to stay in step."""
    h = 14695981039346656037
    for b in data:
        h = ((h ^ b) * 1099511628211) & 0xFFFFFFFFFFFFFFFF
    return h


def resolve_href(referrer, href):
    """Where an href in a .xdb points, as a lowercase pak-relative path.

    Absolute against the data root when it starts with a slash, relative to the
    referring record otherwise, which is how the engine's own resolver reads
    them. Matching on the basename alone instead is what makes a lookup of
    whole.xdb answer with every bridge in the game: the names in this data are
    not unique and the paths are.
    """
    target = href.split('#')[0].replace('\\', '/').strip()
    if not target.lower().endswith('.xdb'):
        return None
    if target.startswith('/'):
        return posixpath.normpath(target[1:]).lower()
    return posixpath.normpath(posixpath.join(posixpath.dirname(referrer), target)).lower()


def pak_signature(paks):
    return [[p, os.path.getsize(p), int(os.path.getmtime(p))] for p in paks]


def find_paks(roots):
    out = []
    for root in roots:
        if not os.path.exists(root):
            continue
        if os.path.isfile(root) and root.lower().endswith('.pak'):
            out.append(root)
            continue
        for dirpath, _dirnames, filenames in os.walk(root):
            for name in sorted(filenames):
                if name.lower().endswith('.pak'):
                    out.append(os.path.join(dirpath, name))
    return sorted(set(out))


def build_index(paks):
    """One pass over every pak, producing the three lookups this needs.

    Kept as one pass because the expensive part is decompressing 65,000 zip
    entries, and doing that three times to build three indexes would take three
    times as long for no benefit.
    """
    hashes = {}       # hash -> [pak::entry]
    uids = {}         # GUID (upper) -> [pak::entry of the .xdb declaring it]
    referrers = {}    # lowercase resolved .xdb path -> [pak::entry referencing it]

    for n, pak in enumerate(paks, 1):
        print('  [%d/%d] %s' % (n, len(paks), os.path.basename(pak)), file=sys.stderr)
        try:
            z = zipfile.ZipFile(pak)
        except (zipfile.BadZipFile, OSError) as e:
            print('        skipped: %s' % e, file=sys.stderr)
            continue
        for info in z.infolist():
            name = info.filename
            lower = name.lower()
            label = '%s::%s' % (pak, name)
            if lower.startswith(GR2_DIRECTORIES):
                try:
                    data = z.read(name)
                except (zipfile.BadZipFile, RuntimeError, OSError):
                    continue
                hashes.setdefault('%016x' % fnv1a64(data), []).append(label)
            elif lower.endswith('.xdb'):
                try:
                    data = z.read(name)
                except (zipfile.BadZipFile, RuntimeError, OSError):
                    continue
                for uid in UID_RE.findall(data):
                    uids.setdefault(uid.decode('ascii').upper(), []).append(label)
                for href in set(HREF_RE.findall(data)):
                    target = resolve_href(name, href.decode('utf-8', 'replace'))
                    if target is not None:
                        referrers.setdefault(target, []).append(label)
    return {'hashes': hashes, 'uids': uids, 'referrers': referrers}


def load_index(roots, refresh):
    paks = find_paks(roots)
    if not paks:
        raise SystemExit('no .pak found under %s' % ', '.join(roots))
    signature = pak_signature(paks)
    if not refresh and os.path.exists(CACHE_PATH):
        try:
            with open(CACHE_PATH, encoding='utf-8') as f:
                cached = json.load(f)
            if cached.get('signature') == signature:
                return cached['index']
        except (ValueError, OSError, KeyError):
            pass
    print('indexing %d paks, once:' % len(paks), file=sys.stderr)
    index = build_index(paks)
    os.makedirs(os.path.dirname(CACHE_PATH), exist_ok=True)
    with open(CACHE_PATH, 'w', encoding='utf-8') as f:
        json.dump({'signature': signature, 'index': index}, f)
    print('  cached in %s' % CACHE_PATH, file=sys.stderr)
    return index


def entry_of(label):
    return label.split('::', 1)[1] if '::' in label else label


def guid_of(label):
    """The resource GUID a pak entry is named after."""
    name = entry_of(label).rsplit('/', 1)[-1]
    return name.upper() if re.fullmatch(r'[0-9A-Fa-f-]{36}', name) else None


def walk_up(index, labels, depth):
    """Who references these .xdb records, and who references those."""
    out = []
    seen = set(labels)
    frontier = list(labels)
    for level in range(depth):
        nxt = []
        for label in frontier:
            path = entry_of(label).replace('\\', '/').lower()
            for referrer in index['referrers'].get(path, []):
                if referrer in seen:
                    continue
                seen.add(referrer)
                nxt.append(referrer)
                out.append((level + 1, referrer))
        if not nxt:
            break
        frontier = nxt
    return out


def report(index, want, depth, limit):
    for h in want:
        print('\n%s' % h)
        labels = index['hashes'].get(h.lower())
        if not labels:
            print('  no pak entry has these bytes. A mod, another install, or an '
                  'index built over the wrong game.')
            continue
        for label in labels:
            print('  %s' % entry_of(label))
            print('    in %s' % label.split('::', 1)[0])
            guid = guid_of(label)
            if guid is None:
                continue
            records = index['uids'].get(guid, [])
            if not records:
                print('    no .xdb declares this GUID')
                continue
            for record in records:
                print('    declared by %s' % entry_of(record))
                for level, referrer in walk_up(index, [record], depth)[:limit]:
                    print('      %sreferenced by %s' % ('  ' * (level - 1),
                                                        entry_of(referrer)))


def hashes_from_log(path, warned_only):
    """The file hashes a run recorded, in the order it loaded them.

    With --warned, only the files a warning named, which is the short list worth
    looking at after a run that went wrong.
    """
    manifest = re.compile(r'\bfile#(\d+) ([0-9a-f]{16})\b')
    numbers = {}
    order = []
    warned = set()
    with open(path, encoding='utf-8', errors='replace') as f:
        for line in f:
            for number, digest in manifest.findall(line):
                numbers[number] = digest
                if digest not in order:
                    order.append(digest)
            if '[warning]' in line or '[error]' in line:
                for number in re.findall(r'\bfile#(\d+)\b', line):
                    warned.add(number)
    if warned_only:
        return [numbers[n] for n in sorted(warned, key=int) if n in numbers]
    return order


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('hashes', nargs='*', help='16 hex digits, as the log prints them')
    parser.add_argument('--log', help='read the hashes out of a libgr2 log instead')
    parser.add_argument('--warned', action='store_true',
                        help='with --log, only the files a warning or an error named')
    parser.add_argument('--roots', nargs='*', default=gr2diff.DEFAULT_CORPUS,
                        help='game installs or paks to index')
    parser.add_argument('--refresh', action='store_true', help='rebuild the index')
    parser.add_argument('--depth', type=int, default=2,
                        help='levels of .xdb reference to walk up (default 2)')
    parser.add_argument('--limit', type=int, default=12,
                        help='referencing records to print per resource')
    args = parser.parse_args(argv[1:])

    want = list(args.hashes)
    if args.log:
        want += hashes_from_log(args.log, args.warned)
    if not want:
        parser.error('give a hash, or --log to read them out of one')

    index = load_index(args.roots, args.refresh)
    report(index, want, args.depth, args.limit)
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
