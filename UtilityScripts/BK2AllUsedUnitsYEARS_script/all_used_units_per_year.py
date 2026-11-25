import math
import os
import utils
from folder_system import FolderSystem
from pak_loader import PakLoader
from console_logger import ConsoleLogger
from virtual_file_system import VirtualFileSystem
from lxml import objectify
from utils import tuple_sum, tuple_sub, tuple_average
import bk2_map_xml_utils
import bk2_xml_utils
import argparse


CUSTOM_REINF_ORDER_PRIOTITIES = {
	"RT_HEAVY_TANKS": -4,
	"RT_TANK_DESTROYERS": -3,
	"RT_TANKS": -2,
	"RT_LIGHT_TANKS": -1,
	"RT_MAIN_INFANTRY": 0,
	"RT_ASSAULT_INFANTRY": 1,
	"RT_PARATROOPS": 2,
	"RT_ELITE_INFANTRY": 3,
	"RT_ENGINEERING": 4,
	"RT_ARTILLERY_ANTITANK": 5,
	"RT_LIGHT_AAA": 6,
	"RT_HEAVY_AAA": 7,
	"RT_ARTILLERY": 8,
	"RT_ASSAULT_GUNS": 9,
	"RT_HEAVY_ARTILLERY": 10,
	"RT_ARTILLERY_ROCKET": 11,
	"RT_FIGHTERS": 12,
	"RT_GROUND_ATTACK_PLANES": 13,
	"RT_BOMBERS": 14,
	"RT_RECON": 15,
}


class UnitPositioning:

	spawn_pos_start = (700, 700, 0)

	unit_span = (15500, 15500)

	unit_spacing_x = 120
	unit_spacing_y = 210

	@staticmethod
	def __place_unit(unit, map, pos, direction=32768, player=0):
		bk2_map_xml_utils.add_object_on_map(map, unit.attrib["href"], pos, direction, player)

	@staticmethod
	def __get_placement_pattern(unit_count: int, center_position: tuple) -> list[tuple[float]]:
		# generate pattern
		side = math.floor(math.sqrt(unit_count))
		width = math.ceil(unit_count / side)
		counter = 0
		result = []
		for column in range(side):
			tmp = []
			for row in range(width):
				if counter >= unit_count:
					break
				tmp.append((row * UnitPositioning.unit_spacing_x, column * UnitPositioning.unit_spacing_y, 0))
				counter += 1
			# center the row around center_position.x
			avg = tuple_average(tmp)
			for i in range(len(tmp)):
				# tmp[i][0] -= avg[0]
				tmp[i] = (tmp[i][0] - avg[0], tmp[i][1], tmp[i][2])
			result.extend(tmp)

		# place result on the center_position
		avg = tuple_average(result)
		diff = tuple_sub(center_position, avg)
		for i in range(len(result)):
			result[i] = tuple_sum(result[i], diff)

		return result

	@staticmethod
	def __get_position(tech_level: int, reinf: int, unit: int, unit_count: int, x_prog: float, y_prog: float)\
			-> (float, float, float):
		base_position = (reinf * x_prog, tech_level * y_prog, 0.0)
		# result = (reinf * x_prog + unit * UnitPositioning.unit_spacing, tech_level * y_prog, 0.0)

		pattern = UnitPositioning.__get_placement_pattern(unit_count, base_position)
		result = pattern[unit]
		# result = base_position

		result = tuple_sum(result, UnitPositioning.spawn_pos_start)
		return result

	@staticmethod
	def place_units(units: list[list[list]], reinf_types: list[str], map):
		y_progression = UnitPositioning.unit_span[1] / len(units)
		for i, side in enumerate(units):
			if len(side) < 1:
				continue
			x_progression = UnitPositioning.unit_span[0] / len(side)
			for j, reinf in enumerate(side):
				for k, unit in enumerate(side[reinf]):
					position = UnitPositioning.__get_position(i, j, k, len(side[reinf]), x_progression, y_progression)
					UnitPositioning.__place_unit(unit, map, position)
		return


def sort_reinfs_by_type(fs: VirtualFileSystem, reinfs: list[str]) -> list[str]:
	ret = []
	for reinf_ref in reinfs:
		reinf_content = bk2_xml_utils.href_get_binary_file_contents(reinf_ref, fs)
		reinf = objectify.fromstring(reinf_content)
		reinf_type = str(reinf.Type)
		ret.append((reinf_ref, reinf_type))
	return ret


def custom_reinf_sorting(a):
	if a in CUSTOM_REINF_ORDER_PRIOTITIES:
		return CUSTOM_REINF_ORDER_PRIOTITIES[a]
	return 0


def get_all_used_reinf_types(fs: VirtualFileSystem, consts, t) -> list[str]:
	ret = set()
	for i, side in enumerate(consts.Sides.Item):
		level = side.TechLevels.Item[t]
		reinfs = level.Reinforcements.Item
		for reinf_ref in reinfs:
			reinf_content = bk2_xml_utils.href_get_binary_file_contents(reinf_ref, fs)
			reinf = objectify.fromstring(reinf_content)
			reinf_type = str(reinf.Type)
			ret.add(reinf_type)
	return list(sorted(ret, key=custom_reinf_sorting))


def main(args):

	path1 = args.datafolder
	path2 = args.modfolder
	map_folder = args.mapfolder

	map_template = os.path.join(map_folder, 'mapinfo.xdb')

	logger = ConsoleLogger()


	if path2 is not None:
		folder = FolderSystem(path2)
		folder.add_directory(path1)
	else:
		folder = FolderSystem(path1)

	fs = VirtualFileSystem(folder)
	pk = PakLoader(path1, logger)
	if path2 is not None:
		pk.open_directory(path2)
	fs.add_system(pk)

	consts = bk2_xml_utils.load_xml_file(fs, "Consts/Test/Test_MultiplayerConsts.xdb")

	map_name = "AllUsedUnitsPerYear"

	counter = 0
	for t, tech_level in enumerate(consts.TechLevels.Item):
		counter += 1
		units = []
		map = bk2_xml_utils.load_xml_file(fs, map_template)
		used_reinf_types = get_all_used_reinf_types(fs, consts, t)
		for i, side in enumerate(consts.Sides.Item):
			units.append([])
			map.Players.Item[0].PartyInfo.attrib["href"] = side.PartyInfo.attrib["href"]
			level = side.TechLevels.Item[t]
			reinfs = level.Reinforcements.Item
			reinfs = sort_reinfs_by_type(fs, reinfs)
			# print(f"Side: {i}, reinfs: ")

			units[i] = dict()
			for used_type in used_reinf_types:
				if used_type not in units[i].keys():
					units[i][used_type] = []

			for k, reinf_pair in enumerate(reinfs):
				reinf_ref = reinf_pair[0]
				reinf_type: str = reinf_pair[1]

				bk2_map_xml_utils.add_reinf_type(map, reinf_ref.attrib["href"], 0)
				reinf_content = bk2_xml_utils.href_get_binary_file_contents(reinf_ref, fs)
				# print(f"Reinf content type: {type(reinf_content)}, path: {reinf_ref.attrib['href']}")
				reinf = objectify.fromstring(reinf_content)
				try:
					reinf_items = reinf.Entries.Item
				except Exception as e:
					print(f"Error: {str(e)} for file {reinf_ref.attrib['href']}")
					continue
				for j, unit in enumerate(reinf_items):
					if bk2_xml_utils.href_file_exists(unit.MechUnit, fs):
						units[i][reinf_type].append(unit.MechUnit)
						continue
					if bk2_xml_utils.href_file_exists(unit.Squad, fs):
						units[i][reinf_type].append(unit.Squad)
						continue

		UnitPositioning.place_units(units, used_reinf_types, map)
		tech_level_name = bk2_xml_utils.href_get_file_contents(tech_level.NameFileRef, fs, "Consts/Test")
		map_name_bytes = utils.string_to_utf16_le(f"{map_name} {tech_level_name}")
		bk2_xml_utils.add_and_write_href_file(
			fs,
			map.LocalizedNameFileRef,
			f"name{counter}.txt",
			map_name_bytes,
			map_folder
		)
		output_path = os.path.join(map_folder, f"mapinfo{counter}.xdb")
		bk2_xml_utils.save_object_as_xml(map, fs, output_path)
	return 0


if __name__ == '__main__':
	parser = argparse.ArgumentParser(
		prog='All Used Units',
		description="Generates a list of all used units in separate map files per years."
	)
	parser.add_argument("--datafolder")
	parser.add_argument("-m", "--modfolder", default=None, type=str)
	# parser.add_argument("-t", "--techlevel", default=None, type=int)
	parser.add_argument("--mapfolder")

	args = parser.parse_args()
	main(args)
