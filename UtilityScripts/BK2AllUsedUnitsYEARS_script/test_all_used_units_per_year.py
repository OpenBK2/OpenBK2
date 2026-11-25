from pathlib import Path
import subprocess
import sys
from tempfile import TemporaryDirectory
import unittest
from xml.etree import ElementTree as ET
from zipfile import ZipFile


SCRIPT = Path(__file__).with_name("all_used_units_per_year.py")


class YearMapsTest(unittest.TestCase):
    def setUp(self):
        temp = TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        self.data = Path(temp.name, "Data")
        self.data.mkdir()
        self.mod = Path(temp.name, "Mod")
        self.maps = self.mod / "Maps"
        self.consts = self.mod / "Consts/Test/Test_MultiplayerConsts.xdb"
        self.consts.parent.mkdir(parents=True)
        root = ET.Element("MultiplayerConsts")
        levels = ET.SubElement(root, "TechLevels")
        for year in (1940, 1941, 1942, 1943):
            item = ET.SubElement(levels, "Item")
            ET.SubElement(item, "NameFileRef", href=f"{year}.txt")
            self.consts.with_name(f"{year}.txt").write_text(str(year), encoding="utf-16")
        sides = ET.SubElement(root, "Sides")
        with ZipFile(self.data / "records.pak", "w") as pak:
            pak.writestr("Maps/mapinfo.xdb", "<MapInfo><Players><Item/></Players><Objects/></MapInfo>")
            for name, availability in (("A", [True]), ("B", [True, False]), ("C", [False, True, True])):
                side = ET.SubElement(sides, "Item")
                ET.SubElement(side, "PartyInfo", href=f"/Parties/{name}.xdb")
                pak.writestr(f"Parties/{name}.xdb", f'<PartyDependentInfo><GeneralPartyName>{name}</GeneralPartyName>'
                             f'<KeyBuildingFlag href="Flags/{name}.xdb#xpointer(/ObjectRPGStats)"/></PartyDependentInfo>')
                pak.writestr(f"Parties/Flags/{name}.xdb", "<ObjectRPGStats/>")
                levels = ET.SubElement(side, "TechLevels")
                for level, enabled in enumerate(availability):
                    item = ET.SubElement(levels, "Item")
                    if not enabled:
                        ET.SubElement(item, "Disabled").text = "true" if name == "B" else "1"
                    reinfs = ET.SubElement(item, "Reinforcements")
                    if level < 2:
                        href = "/Forces/units.xdb" if enabled else "/disabled-must-not-be-read.xdb"
                        ET.SubElement(reinfs, "Item", href=href)
            pak.writestr("Forces/units.xdb", '<Reinforcement><Type>RT_TANKS</Type><Entries>'
                         '<Item><Squad href="squad.xdb#xpointer(/SquadRPGStats)"/></Item></Entries></Reinforcement>')
        # The case in a game href can differ from that of a loose file on Linux.
        (self.data / "Forces").mkdir()
        (self.data / "Forces/Squad.xdb").write_text("<SquadRPGStats/>", encoding="utf-8")
        ET.ElementTree(root).write(self.consts)

    def generate(self, *options):
        return subprocess.run([sys.executable, "-B", str(SCRIPT), "--datafolder", str(self.data), "--modfolder",
                               str(self.mod), "--mapfolder", "Maps", *options], capture_output=True, text=True)

    def placements(self, number):
        root = ET.parse(self.maps / f"mapinfo{number}.xdb")
        self.assertEqual(root.find("LocalizedNameFileRef").get("href"), f"name{number}.txt")
        self.assertEqual((self.maps / f"name{number}.txt").read_text("utf-16"), f"AllUsedUnitsPerYear {1939 + number}")
        objects = root.findall("Objects/Item")
        self.assertEqual([int(o.findtext("Link/LinkID")) for o in objects], list(range(1, len(objects) + 1)))
        return [(o.find("Object").get("href"), float(o.findtext("Pos/x")), float(o.findtext("Pos/y"))) for o in objects]

    def test_shared_rows(self):
        run = self.generate("--row", "c", "1")
        self.assertEqual((run.returncode, run.stderr), (0, ""))

        def flag(name):
            return f"/Parties/Flags/{name}.xdb#xpointer(/ObjectRPGStats)"

        unit = "/Forces/squad.xdb#xpointer(/SquadRPGStats)"
        self.assertEqual(self.placements(1), [(flag("A"), 700, 700), (unit, 8450, 700),
                                             (flag("B"), 700, 8450), (unit, 8450, 8450)])
        self.assertEqual(self.placements(2), [(flag("C"), 700, 8450), (unit, 8450, 8450)])
        self.assertEqual(self.placements(3), [(flag("C"), 700, 8450)])
        self.assertEqual(self.placements(4), [])
        self.assertEqual(ET.parse(self.maps / "mapinfo2.xdb").find("Players/Item/PartyInfo").get("href"),
                         "/Parties/C.xdb")
        reinfs = ET.parse(self.maps / "mapinfo2.xdb").findall("Players/Item/ReinforcementTypes/Item")
        self.assertEqual([r.get("href") for r in reinfs], ["/Forces/units.xdb"])

    def test_default_packs_nations_with_the_same_spacing_in_busy_and_quiet_years(self):
        root = ET.parse(self.consts)
        _, b, c = root.findall("Sides/Item")
        # Include flag-only nations. A/B/C are enabled in 1940 and only B/C in 1941.
        c.findall("TechLevels/Item")[0].clear()
        b.findall("TechLevels/Item")[1].clear()
        root.write(self.consts)
        run = self.generate()
        self.assertEqual((run.returncode, run.stderr), (0, ""))
        self.assertIn("/A.xdb#", self.placements(1)[0][0])
        second_row_y = 700 + 15500 / 3
        self.assertEqual(self.placements(1)[2][1:], (700, second_row_y))
        quiet_year = self.placements(2)
        self.assertIn("/B.xdb#", quiet_year[0][0])
        self.assertEqual(quiet_year[0][1:], (700, 700))
        self.assertIn("/C.xdb#", quiet_year[1][0])
        self.assertEqual(quiet_year[1][1:], (700, second_row_y))
        self.assertEqual(self.placements(3)[0][1:], (700, 700))
        self.assertEqual(self.placements(4), [])

    def test_late_conflict_leaves_output_untouched(self):
        root = ET.parse(self.consts)
        root.findall("Sides/Item")[1].findall("TechLevels/Item")[1].find("Disabled").text = "false"
        root.write(self.consts)
        run = self.generate("--row", "B", "1", "--row", "C", "1")
        self.assertNotEqual(run.returncode, 0)
        for detail in ("Row 1", "tech level 2 (1941)", "B (side 1)", "C (side 2)"):
            self.assertIn(detail, run.stderr)
        self.assertFalse(self.maps.exists())

    def test_row_must_be_inside_peak_grid(self):
        for row in ("-1", "2", "France"):
            with self.subTest(row=row):
                run = self.generate("--row", "C", row)
                self.assertNotEqual(run.returncode, 0)
                self.assertIn(row, run.stderr)
                self.assertFalse(self.maps.exists())

    def test_unassigned_nation_fills_around_pins(self):
        run = self.generate("--row", "B", "0", "--row", "2", "0")
        self.assertEqual((run.returncode, run.stderr), (0, ""))
        self.assertIn("/B.xdb#", self.placements(1)[0][0])
        self.assertIn("/A.xdb#", self.placements(1)[2][0])
        self.assertIn("/C.xdb#", self.placements(2)[0][0])
        self.assertEqual(self.placements(2)[0][1:], (700, 700))

    def test_preserves_existing_objects_and_link_ids(self):
        maps = self.mod / "maps"
        maps.mkdir()
        template = maps / "mapinfo.xdb"
        original = ('<MapInfo><Players><Item/></Players><Objects><Item><Link><LinkID>9</LinkID></Link></Item>'
                    '<Item><Link><LinkID>2</LinkID></Link></Item></Objects></MapInfo>')
        template.write_text(original, encoding="utf-8")
        run = self.generate()
        self.assertEqual((run.returncode, run.stderr), (0, ""))
        self.assertEqual(template.read_text(encoding="utf-8"), original)
        ids = ET.parse(maps / "mapinfo1.xdb").findall("Objects/Item/Link/LinkID")
        self.assertEqual([int(i.text) for i in ids], [9, 2, 10, 11, 12, 13])


if __name__ == "__main__":
    unittest.main()
