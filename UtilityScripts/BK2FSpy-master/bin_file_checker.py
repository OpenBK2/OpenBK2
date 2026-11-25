import os
import utils
from folder_system import FolderSystem
from pak_loader import PakLoader
from console_logger import ConsoleLogger
from virtual_file_system import VirtualFileSystem
from lxml import objectify
from utils import tuple_sum, tuple_scalar_multiply
from set_logger import SetLogger
import bk2_map_xml_utils
import bk2_xml_utils
import argparse


def main(args):

	path1 = args.datafolder
	path2 = args.modfolder

	logger = ConsoleLogger()

	folder = FolderSystem(path2)
	fs = VirtualFileSystem(folder)
	pk = PakLoader(path1, logger)
	pk.open_directory(path2)
	fs.add_system(pk)

	consts = bk2_xml_utils.load_xml_file(fs, "Consts/Test/Test_MultiplayerConsts.xdb")

	logger = SetLogger()

	for i, side in enumerate(consts.Sides.Item):
		for j, techlevel in enumerate(side.TechLevels.Item):
			for k, reinf_ref in enumerate(techlevel.Reinforcements.Item):
				reinf_content = bk2_xml_utils.href_get_binary_file_contents(reinf_ref, fs)
				reinf = objectify.fromstring(reinf_content)
				try:
					reinf_items = reinf.Entries.Item
				except Exception as e:
					# print(f"Error: '{str(e)}' for file '{reinf_ref.attrib['href']}'")
					continue
				files = set()
				path = ""
				for u, unit in enumerate(reinf_items):
					if bk2_xml_utils.href_file_exists(unit.MechUnit, fs):
						files_tmp = set()
						objreader = bk2_xml_utils.VisualObjectReader(fs, files_tmp, logger)
						# print(f"Check path: {path}")
						try:
							path = unit.MechUnit.attrib['href']
							path = bk2_xml_utils.format_href(path)
							objreader.read_RPGStats(path, os.path.dirname(path))
							addition = [(i, path) for i in objreader.result]
							files = files.union(addition)
							addition = [(i, path) for i in files_tmp]
							# files = files.union(addition)

							# weapon recoils check
							rpg_stats = bk2_xml_utils.href_read_xml_object(unit.MechUnit, fs, os.path.dirname(path))
							if hasattr(rpg_stats.platforms, "Item"):
								for p, platform in enumerate(rpg_stats.platforms.Item):
									if hasattr(platform.guns, "Item"):
										for g, gun in enumerate(platform.guns.Item):
											if not gun.Weapon.attrib["href"]:
												continue
											weapon = gun.Weapon
											if not bk2_xml_utils.href_file_exists(weapon, fs, os.path.dirname(path)):
												logger.print(f"Weapon: '{weapon.attrib['href']}' not found for '{path}'"
															 f"for platform[{p}]->gun[{g}]")
												continue
											if bool(gun.Recoil.text):
												if float(gun.RecoilLength) < 0.0:
													logger.print(f"'{path}' has negative recoil length"
																 f" for platform[{p}]->gun[{g}]")
												# if float(gun.RecoilShakeAngle) < 0.0:
												# 	logger.print(f"'{path}' has negative recoil shake angle"
												# 				 f"for platform{p}->gun{g}")

						except Exception as e:
							logger.print(f"{str(e)} for file {path}")
						continue
					if bk2_xml_utils.href_file_exists(unit.Squad, fs):

						# for skipping infantry
						continue

						files_tmp = set()
						objreader = bk2_xml_utils.VisualObjectReader(fs, files_tmp, logger)
						squad_stats = bk2_xml_utils.href_read_xml_object(unit.Squad, fs)
						if not hasattr(squad_stats.members, "Item"):
							continue
						for item in squad_stats.members.Item:
							path = item.attrib['href']
							path = bk2_xml_utils.format_href(path)
							if not path:
								continue
							try:
								objreader.read_RPGStats(path, os.path.dirname(path))
								addition = [(i, path) for i in objreader.result]
								files = files.union(addition)
								addition = [(i, path) for i in files_tmp]
								# files = files.union(addition)
							except Exception as e:
								logger.print(f"'{str(e)}' for file '{path}', fs_exits?: {fs.contains_file(path)}")
						continue
				# print(files)
				for file_path in files:
					file = file_path[0]
					path = file_path[1]
					file = bk2_xml_utils.format_href(file)
					if not file:
						continue
					if not fs.contains_file(file):
						logger.print(f"Error: '{file}' not found for '{path}'!")
						continue
					content = fs.read_file_bytes(file)
					if len(content) < 5:
						logger.print(f"Error: '{file}' is empty for '{path}'!")

	messages = list(logger.messages.keys())
	for message in messages:
		print(message)
						



if __name__ == '__main__':
	parser = argparse.ArgumentParser(
		prog='Bin File Checker',
		description="Checks the validity of bk2 binary (bin/) files of used units in MP."
	)
	parser.add_argument("datafolder")
	parser.add_argument("modfolder")

	args = parser.parse_args()
	main(args)
