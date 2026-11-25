import math
import os
import utils
from folder_system import FolderSystem
from pak_loader import PakLoader
from console_logger import ConsoleLogger
from virtual_file_system import VirtualFileSystem
from lxml import objectify
from utils import tuple_sum, tuple_scalar_multiply, tuple_sub, tuple_average
import bk2_map_xml_utils
import bk2_xml_utils
import argparse


class UnitPositioning:

	spawn_pos_start = (700, 700, 0)

	unit_span = (17000, 17000)

	unit_spacing_x = 120
	unit_spacing_y = 210

	"""reinf_offset = (600, 0, 0)
	unit_offset = (0, 300, 0)
	techlevel_offset = (0, 1000, 0)"""

	"""def get_unit_position(self, tech_level: int, reinf_num: int, reinf_id: int) -> (float, float, float):
		res = tuple_scalar_multiply(UnitPositioning.unit_offset, reinf_num)
		res = tuple_sum(res, tuple_scalar_multiply(UnitPositioning.techlevel_offset, tech_level))
		res = tuple_sum(res, tuple_scalar_multiply(UnitPositioning.reinf_offset, reinf_id))
		return tuple_sum(res, UnitPositioning.spawn_pos_start)"""

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
	def place_units(units: list[list[list]], map):
		y_progression = UnitPositioning.unit_span[1] / len(units)
		for i, techlevel in enumerate(units):
			x_progression = UnitPositioning.unit_span[0] / len(techlevel)
			for j, reinf in enumerate(techlevel):
				for k, unit in enumerate(reinf):
					position = UnitPositioning.__get_position(i, j, k, len(reinf), x_progression, y_progression)
					UnitPositioning.__place_unit(unit, map, position)
		return


def main(args):

	path1 = args.datafolder
	path2 = args.modfolder
	map_folder = args.mapfolder
	map_template = os.path.join(map_folder, 'mapinfo.xdb')

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

	nation_ids = [0, 1, 2, 3, 4, 5, 6]
	tech_levels = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11]

	map_name = "AllUsedUnits"

	counter = 0
	for i, side in enumerate(consts.Sides.Item):
		counter += 1
		units = []
		map = bk2_xml_utils.load_xml_file(fs, map_template)
		for t, tech_level in enumerate(consts.TechLevels.Item):
			units.append([])
			map.Players.Item[0].PartyInfo.attrib["href"] = side.PartyInfo.attrib["href"]
			level = side.TechLevels.Item[t]
			reinfs = level.Reinforcements.Item
			# print(f"Side: {i}, reinfs: ")
			for k, reinf_ref in enumerate(reinfs):
				units[t].append([])
				bk2_map_xml_utils.add_reinf_type(map, reinf_ref.attrib["href"], 0)
				reinf_content = bk2_xml_utils.href_get_binary_file_contents(reinf_ref, fs)
				reinf = objectify.fromstring(reinf_content)
				try:
					reinf_items = reinf.Entries.Item
				except Exception as e:
					print(f"Error: {str(e)} for file {reinf_ref.attrib['href']}")
					continue
				for j, unit in enumerate(reinf_items):
					if bk2_xml_utils.href_file_exists(unit.MechUnit, fs):
						# bk2_map_xml_utils.add_object_on_map(map, unit.MechUnit.attrib["href"], pos, 32768, 0)
						units[t][k].append(unit.MechUnit)
						continue
					if bk2_xml_utils.href_file_exists(unit.Squad, fs):
						# bk2_map_xml_utils.add_object_on_map(map, unit.Squad.attrib["href"], pos, 32768, 0)
						units[t][k].append(unit.Squad)
						continue
		UnitPositioning.place_units(units, map)
		map_name_bytes = utils.string_to_utf16_le(f"{map_name}{counter}")
		bk2_xml_utils.add_and_write_href_file(fs, map.LocalizedNameFileRef, f"name{counter}.txt", map_name_bytes, map_folder)
		output_path = os.path.join(map_folder, f"mapinfo{counter}.xdb")
		bk2_xml_utils.save_object_as_xml(map, fs, output_path)
	return 0


if __name__ == '__main__':
	parser = argparse.ArgumentParser(
		prog='All Used Units',
		description="Genereates a list of all used units in separate map files."
	)
	parser.add_argument("datafolder")
	parser.add_argument("modfolder")
	parser.add_argument("mapfolder")

	args = parser.parse_args()
	main(args)
