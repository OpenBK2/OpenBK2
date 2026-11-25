import utils
from folder_system import FolderSystem
from pak_loader import PakLoader
from console_logger import ConsoleLogger
from virtual_file_system import VirtualFileSystem
from lxml import objectify
from utils import tuple_sum, tuple_scalar_multiply
import bk2_map_xml_utils
import bk2_xml_utils


path1 = r"C:\Program Files (x86)\Steam\steamapps\common\Blitzkrieg 2 Anthology\Blitzkrieg 2\Data"
path2 = r"C:\Program Files (x86)\Steam\steamapps\common\Blitzkrieg 2 Anthology\Blitzkrieg 2\mods\Universal MOD-18 Xitest"


def main():

    global path1
    global path2

    logger = ConsoleLogger()

    folder = FolderSystem(path2)
    fs = VirtualFileSystem(folder)
    pk = PakLoader(path1, logger)
    pk.open_directory(path2)
    fs.add_system(pk)

    consts = bk2_xml_utils.load_xml_file(fs, "Consts/Test/Test_MultiplayerConsts.xdb")
    """print("Sides:")
    for i, side in enumerate(consts.Sides.Item):
        side_name_ref = side.NameFileRef
        print(str(i) + " " + bk2_xml_utils.href_get_file_contents(side_name_ref, fs))

    print("Tech levels:")
    for i, tech_level in enumerate(consts.TechLevels.Item):
        name_ref = tech_level.NameFileRef
        print(str(i) + " " + bk2_xml_utils.href_get_file_contents(name_ref, fs, "Consts/Test"))"""

    nation_ids = [1, 2, 3, 5, 6]
    tech_level = 6

    map_name = "ReinfInspection"

    map_folder = r"Custom/Missions/ReinfInspection_Template/"
    map_template = r"Custom/Missions/ReinfInspection_Template/mapinfo.xdb"

    done_ids = set()

    spawn_pos_start = (450, 200, 0)

    counter = 0
    for i, side in enumerate(consts.Sides.Item):
        if i in nation_ids and i not in done_ids:
            done_ids.add(i)
            counter += 1

            map = bk2_xml_utils.load_xml_file(fs, map_template)

            spawn_pos = spawn_pos_start
            reinf_offset = (180, 0, 0)
            unit_offset = (0, 225, 0)

            map.Players.Item[0].PartyInfo.attrib["href"] = side.PartyInfo.attrib["href"]
            level = side.TechLevels.Item[tech_level]
            reinfs = level.Reinforcements.Item
            # print(f"Side: {i}, reinfs: ")
            for reinf_ref in reinfs:
                bk2_map_xml_utils.add_reinf_type(map, reinf_ref.attrib["href"], 0)
                reinf_content = bk2_xml_utils.href_get_binary_file_contents(reinf_ref, fs)
                reinf = objectify.fromstring(reinf_content)
                for j, unit in enumerate(reinf.Entries.Item):
                    pos = tuple_sum(spawn_pos, tuple_scalar_multiply(unit_offset, j))
                    if bk2_xml_utils.href_file_exists(unit.MechUnit, fs):
                        bk2_map_xml_utils.add_object_on_map(map, unit.MechUnit.attrib["href"], pos, 32000, 0)
                        continue
                    if bk2_xml_utils.href_file_exists(unit.Squad, fs):
                        bk2_map_xml_utils.add_object_on_map(map, unit.Squad.attrib["href"], pos, 32000, 0)
                        continue
                spawn_pos = tuple_sum(spawn_pos, reinf_offset)
            map_name_bytes = utils.string_to_utf16_le(f"{map_name}{counter}")
            bk2_xml_utils.add_and_write_href_file(fs, map.LocalizedNameFileRef, f"name{counter}.txt", map_name_bytes,
                                                  map_folder)
            bk2_xml_utils.save_object_as_xml(map, fs, fr"Custom\Missions\ReinfInspection_Template\mapinfo{counter}.xdb")
    return 0


if __name__ == '__main__':
    main()
