import argparse
from dataclasses import dataclass
from itertools import count
import math
import os
import posixpath
import sys

from lxml import etree

import bk2_map_xml_utils
import bk2_xml_utils
from console_logger import ConsoleLogger
from folder_system import FolderSystem
from pak_loader import PakLoader
from utils import string_to_utf16_le
from virtual_file_system import VirtualFileSystem


MP_CONSTS = "Consts/Test/Test_MultiplayerConsts.xdb"
REINF_ORDER = (
    "RT_HEAVY_TANKS", "RT_TANK_DESTROYERS", "RT_TANKS", "RT_LIGHT_TANKS",
    "RT_MAIN_INFANTRY", "RT_ASSAULT_INFANTRY", "RT_PARATROOPS", "RT_ELITE_INFANTRY",
    "RT_ENGINEERING", "RT_ARTILLERY_ANTITANK", "RT_LIGHT_AAA", "RT_HEAVY_AAA",
    "RT_ARTILLERY", "RT_ASSAULT_GUNS", "RT_HEAVY_ARTILLERY", "RT_ARTILLERY_ROCKET",
    "RT_FIGHTERS", "RT_GROUND_ATTACK_PLANES", "RT_BOMBERS", "RT_RECON",
)


def resolve_href(fs, element, source):
    """Rebase a reference from its owning XDB so it also works in the output map."""
    href = element.get("href", "").strip() if element is not None else ""
    if not href:
        return None
    path, fragment, pointer = href.replace("\\", "/").partition("#")
    source_dir = posixpath.dirname(source.partition("#")[0])
    for candidate in (posixpath.join(source_dir, path), path):
        rooted = posixpath.normpath("/" + candidate.lstrip("/"))
        if fs.contains_file(rooted[1:]):
            return rooted + fragment + pointer
    print(f"Warning: {source} references missing file {href}", file=sys.stderr)
    return None


def read_xml(fs, href):
    path = href.lstrip("/").partition("#")[0]
    try:
        return bk2_xml_utils.load_xml_file(fs, path)
    except etree.XMLSyntaxError as error:
        raise ValueError(f"{path}: {error}") from error


@dataclass
class Side:
    name: str
    party_href: str | None
    flag_href: str | None
    levels: list

    def available(self, level):
        # The game defaults Disabled to false, but a missing tech level is unavailable.
        if level >= len(self.levels):
            return False
        disabled = self.levels[level].findtext("Disabled", "false")
        return disabled.strip().lower() not in ("true", "1")


def read_sides(fs, consts):
    sides = []
    for index, item in enumerate(consts.findall("Sides/Item")):
        name = f"Side {index}"
        party_href = resolve_href(fs, item.find("PartyInfo"), MP_CONSTS)
        flag_href = None
        if party_href:
            party = read_xml(fs, party_href)
            name = party.findtext("GeneralPartyName") or name
            flag_href = resolve_href(fs, party.find("KeyBuildingFlag"), party_href)
        side = Side(name, party_href, flag_href, item.findall("TechLevels/Item"))
        if not flag_href and any(side.available(i) for i in range(len(side.levels))):
            print(f"Warning: {name} (side {index}) has no usable KeyBuildingFlag", file=sys.stderr)
        sides.append(side)
    return sides


def plan_rows(sides, level_names, row_options):
    groups = []
    assigned = set()
    for options in row_options:
        group = []
        for token in options:
            if token.isdecimal():
                index = int(token)
                if index >= len(sides):
                    raise ValueError(f"Unknown side index {index} (there are {len(sides)} sides)")
            else:
                matches = [i for i, side in enumerate(sides) if side.name.casefold() == token.casefold()]
                if len(matches) != 1:
                    raise ValueError(f"Unknown or ambiguous nation {token!r}. Use its zero-based Sides/Item index.")
                index = matches[0]
            if index in assigned:
                raise ValueError(f"{sides[index].name} (side {index}) occurs more than once in --row")
            assigned.add(index)
            group.append(index)
        groups.append(group)

    # Validate every year before writing the first map. Configured rows keep their
    # slots across years, and the other enabled nations fill in below them.
    layouts = []
    for level, name in enumerate(level_names):
        rows = []
        for row, group in enumerate(groups, start=1):
            active = [i for i in group if sides[i].available(level)]
            if len(active) > 1:
                nations = ", ".join(f"{sides[i].name} (side {i})" for i in active)
                raise ValueError(f"Row {row} has multiple enabled nations at tech level {level + 1} ({name}): {nations}")
            rows.append(active[0] if active else None)
        rows.extend(i for i, side in enumerate(sides) if i not in assigned and side.available(level))
        layouts.append(rows)
    return layouts


def collect_units(fs, level, player):
    units = {}
    for ref in level.findall("Reinforcements/Item"):
        href = resolve_href(fs, ref, MP_CONSTS)
        if not href:
            continue
        reinf = read_xml(fs, href)
        reinf_type = reinf.findtext("Type", "RT_TANKS")  # SReinforcement's database default.
        bk2_map_xml_utils.add_reinf_type(player, href)
        cell = units.setdefault(reinf_type, [])
        for entry in reinf.findall("Entries/Item"):
            for field in ("MechUnit", "Squad"):
                unit = resolve_href(fs, entry.find(field), href)
                if unit:
                    cell.append(unit)
                    break
    return units


def unit_positions(size, x, y):
    # Center each line horizontally and the whole formation vertically, as in the original map.
    width = math.ceil(size / math.isqrt(size))
    mean_line = sum(i // width for i in range(size)) / size
    for start in range(0, size, width):
        length = min(width, size - start)
        for column in range(length):
            yield (x + (column - (length - 1) / 2) * 120, y + (start // width - mean_line) * 210, 0)


def populate_map(fs, map_xml, sides, rows, row_count, level):
    player = map_xml.find("Players/Item")
    if player is None:
        raise ValueError("The map template needs a player in Players/Item")

    # Keep the original player and reinforcement ordering even when rows are reordered.
    units = {}
    for index in sorted(i for i in rows if i is not None):
        side = sides[index]
        if side.party_href:
            bk2_map_xml_utils.child(player, "PartyInfo").set("href", side.party_href)
        units[index] = collect_units(fs, side.levels[level], player)

    types = {kind for nation in units.values() for kind in nation}
    columns = [kind for kind in REINF_ORDER if kind in types] + sorted(types.difference(REINF_ORDER))
    x_step = 15500 / (len(columns) + 1)  # Column zero holds the nation's flagstaff.
    objects = bk2_map_xml_utils.child(map_xml, "Objects")
    link_ids = count(1 + max((int(link.text) for link in objects.iterfind("Item/Link/LinkID")), default=0))
    for row, index in enumerate(rows):
        if index is None:
            continue
        y = 700 + row * 15500 / row_count
        if sides[index].flag_href:
            bk2_map_xml_utils.add_object_on_map(objects, sides[index].flag_href, (700, y, 0), next(link_ids))
        for column, kind in enumerate(columns, start=1):
            cell = units[index].get(kind)
            if cell:
                for href, pos in zip(cell, unit_positions(len(cell), 700 + column * x_step, y)):
                    bk2_map_xml_utils.add_object_on_map(objects, href, pos, next(link_ids))


def main(args):
    folder = FolderSystem(args.modfolder or args.datafolder)
    paks = PakLoader(args.datafolder, ConsoleLogger())
    if args.modfolder:
        folder.add_directory(args.datafolder)
        paks.open_directory(args.modfolder)
    fs = VirtualFileSystem(folder)
    fs.add_system(paks)

    consts = read_xml(fs, MP_CONSTS)
    sides = read_sides(fs, consts)
    level_names = []
    for level in consts.findall("TechLevels/Item"):
        href = resolve_href(fs, level.find("NameFileRef"), MP_CONSTS)
        name = fs.read_text_file(href[1:]) if href else f"Tech level {len(level_names) + 1}"
        level_names.append(name)
    layouts = plan_rows(sides, level_names, args.row)
    max_rows = max(map(len, layouts), default=0)
    template = os.path.join(args.mapfolder, "mapinfo.xdb")
    for level, rows in enumerate(layouts):
        map_xml = bk2_xml_utils.load_xml_file(fs, template)
        populate_map(fs, map_xml, sides, rows, max_rows if args.row else len(rows), level)
        number = level + 1
        bk2_xml_utils.add_and_write_href_file(
            fs, bk2_map_xml_utils.child(map_xml, "LocalizedNameFileRef"), f"name{number}.txt",
            string_to_utf16_le(f"AllUsedUnitsPerYear {level_names[level]}"), args.mapfolder,
        )
        bk2_xml_utils.save_object_as_xml(map_xml, fs, os.path.join(args.mapfolder, f"mapinfo{number}.xdb"))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate a multiplayer unit documentation map for each tech level.")
    parser.add_argument("--datafolder", required=True)
    parser.add_argument("-m", "--modfolder")
    parser.add_argument("--mapfolder", required=True, help="Folder containing the template mapinfo.xdb")
    parser.add_argument("--row", action="append", nargs="+", metavar="SIDE", default=[],
                        help="Reserve one row for these mutually exclusive nations (GeneralPartyName or zero-based "
                             "Sides/Item index). Repeat for more rows. Other active nations follow in side order.")
    try:
        main(parser.parse_args())
    except (ValueError, OSError) as error:
        parser.exit(1, f"{parser.prog}: {error}\n")
