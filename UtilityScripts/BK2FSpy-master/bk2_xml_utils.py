import lxml.etree
from lxml import objectify
from lxml.etree import tostring
from virtual_file_system_abstract import VirtualFileSystemBaseClass
from utils import is_slash
from logger_abstract import LoggerAbstract
import os


def load_xml_file(system: VirtualFileSystemBaseClass, path: str):
	content = system.read_file_bytes(path)
	return objectify.fromstring(content)


def save_object_as_xml(xml_object, system: VirtualFileSystemBaseClass, path: str):
	objectify.deannotate(xml_object)
	lxml.etree.cleanup_namespaces(xml_object)
	content = tostring(xml_object, pretty_print=True, doctype='<?xml version="1.0" encoding="UTF-8" ?>')
	if system is not None:
		system.write_to_file(path, content)
		return
	with open(path, "wb") as file:
		file.write(content)


def format_href(href: str, remove_extension=True):
	if not href.strip():
		return ""
	if is_slash(href[0]):
		href = href[1:]
	href = href.strip().replace("\\", "/").lower()
	if remove_extension and "#" in href:
		index = href.index("#")
		href = href if index < 0 else href[0:index]
	href = href.replace("\\", "/").lower()
	return href


def add_and_write_href_file(system: VirtualFileSystemBaseClass, href_element, file_path: str, file_content: bytes,
							abs_path: str = ""):
	full_path = os.path.join(abs_path, file_path)
	system.write_to_file(full_path, file_content)
	href_element.attrib["href"] = file_path


def add_root_directory(href: str, root_directory: str):
	href = href.replace("\\", "/").lower()
	if not root_directory:
		return href
	dir = os.path.join(root_directory.lower(), href).replace("\\", "/")
	return dir


def href_file_exists(href_object, system: VirtualFileSystemBaseClass, root_directory: str=None, remove_extension=True):
	href = href_object.attrib["href"]
	href = format_href(href, remove_extension)
	if not href:
		return False
	directory = add_root_directory(href, root_directory)
	if system.contains_file(directory):
		return True
	if system.contains_file(href):
		return True
	return False


def actual_path(path: str, system: VirtualFileSystemBaseClass, root_directory: str=None, remove_extension=True):
	path = format_href(path, remove_extension)
	directory = add_root_directory(path, root_directory)
	if system.contains_file(directory):
		return directory
	if system.contains_file(path):
		return path
	raise ValueError(f"href file not found for {path}, root_dir: {root_directory}")


def actual_href_path(href_object, system: VirtualFileSystemBaseClass, root_directory: str=None, remove_extension=True):
	href = href_object.attrib["href"]
	return actual_path(href, system, root_directory, remove_extension)


def href_get_file_contents(href_object, system: VirtualFileSystemBaseClass, root_directory: str=None,
							remove_extension=True, logger: LoggerAbstract=None):
	if not root_directory:
		root_directory = ""
	href = href_object.attrib["href"]
	href = format_href(href, remove_extension)
	if not href:
		return None
	directory = add_root_directory(href, root_directory)
	file_content = None
	try:
		file_content = system.read_text_file(directory)
	except Exception as e:
		logger.print(e) if logger is not None else None
	if file_content is not None:
		return file_content
	try:
		file_content = system.read_text_file(href)
	except Exception as e:
		logger.print(e) if logger is not None else None
	if file_content is not None:
		return file_content
	raise Exception(f"File: {href} not found!")


def href_get_binary_file_contents(href_object, system: VirtualFileSystemBaseClass, root_directory: str=None,
									remove_extension=True, logger: LoggerAbstract=None):
	if not root_directory:
		root_directory = ""
	href = href_object.attrib["href"]
	href = format_href(href, remove_extension)
	if not href:
		return None
	directory = add_root_directory(href, root_directory)
	if system.contains_file(directory):
		return system.read_file_bytes(directory)
	if system.contains_file(href):
		return system.read_file_bytes(href)
	# raise Exception(f"File: {href} not found!")
	return None


def href_read_xml_object(href_object, system: VirtualFileSystemBaseClass, root_directory: str=None,
						remove_extension=True, logger: LoggerAbstract=None):
	contents = href_get_binary_file_contents(href_object, system, root_directory, remove_extension, logger)
	if not contents:
		logger.print(f"File: {href_object.attrib['href']} not found! root_dir: {root_directory}")
		return None
	return objectify.fromstring(contents)


def copy_file_to_folder(file_system: VirtualFileSystemBaseClass, source: str, destination_folder: str) -> bool:
	if not file_system.contains_file(source):
		return False
	destination = os.path.join(destination_folder, source)
	destination_folder = os.path.dirname(destination)
	if not os.path.exists(destination_folder):
		os.makedirs(destination_folder)
	with open(destination, "wb") as f:
		f.write(file_system.read_file_bytes(source))
	return True


class VisualObjectReader:

	def __init__(self, system: VirtualFileSystemBaseClass, used_file_paths: set[str]=None, logger: LoggerAbstract=None):
		self.file_system = system
		self.xml_object = None
		self.root_directory = None
		self.used_file_paths = used_file_paths
		self.logger = logger
		self.result: set[str] | None = None

	def read_RPGStats(self, path, root_directory: str=None, export_unit_weapons: bool=False):
		file_xml = load_xml_file(self.file_system, path)
		root_directory = self.__update_root_directory(path, root_directory)
		self.__read_binaries(file_xml, root_directory, format_href(path), export_unit_weapons)

	def __update_root_directory(self, file_path: str, root_directory: str=None):
		if type(file_path) is not str:
			file_path = file_path.attrib["href"]
		if not root_directory or not root_directory.strip():
			return file_path
		root_dir = format_href(root_directory)
		file_path = format_href(file_path)
		file_dir = os.path.dirname(file_path)
		if not file_dir:
			return root_dir
		return file_dir

	def __add_used_file_path(self, href_element, root_directory: str= ""):
		if self.used_file_paths is not None:
			if type(href_element) is str and self.file_system.contains_file(href_element):
				self.used_file_paths.add(actual_path(href_element, self.file_system, root_directory))
				return
			if href_file_exists(href_element, self.file_system, root_directory):
				self.used_file_paths.add(actual_href_path(href_element, self.file_system, root_directory))

	def __add_used_file_path_if_possible(self, href_element, attribute: str, root_directory: str=""):
		if hasattr(href_element, attribute):
			self.__add_used_file_path(getattr(href_element, attribute), root_directory)

	def __read_graphics_if_possible(self, xml_root, graphic_name: str, root_directory: str=None):
		if hasattr(xml_root, graphic_name):
			self.__read_object_graphics(getattr(xml_root, graphic_name), root_directory)

	def __read_binaries(self, xml_object, root_directory: str = "", element_path: str = "", read_weapons: bool = False):
		self.xml_object = xml_object
		self.root_directory = root_directory
		self.result = set()

		# Damage levels
		if hasattr(xml_object.DamageLevels, "Item"):
			for item in xml_object.DamageLevels.Item:
				self.__read_object_graphics(item.VisObj, root_directory)

		# Visual Object
		# self.__read_object_graphics(xml_object.visualObject, root_directory)
		self.__read_graphics_if_possible(xml_object, "visualObject", root_directory)

		# info Visual Object
		# self.__read_object_graphics(xml_object.infoVisualObject, root_directory)
		self.__read_graphics_if_possible(xml_object, "infoVisualObject", root_directory)

		# Animable Model
		# self.__read_object_graphics(xml_object.AnimableModel, root_directory)
		self.__read_graphics_if_possible(xml_object, "AnimableModel", root_directory)

		# Transportable Model
		# self.__read_object_graphics(xml_object.TransportableModel, root_directory)
		self.__read_graphics_if_possible(xml_object, "TransportableModel", root_directory)

		self.__read_texture(xml_object.IconTexture, root_directory)

		self.__add_used_file_path(element_path, root_directory)
		self.__add_used_file_path_if_possible(xml_object, "LocalizedNameFileRef", root_directory)
		self.__add_used_file_path_if_possible(xml_object, "FullDescriptionFileRef", root_directory)

		if read_weapons and hasattr(xml_object, "platforms") and hasattr(xml_object.platforms, "Item"):
			for item in xml_object.platforms.Item:
				if hasattr(item.guns, "Item"):
					for gun in item.guns.Item:
						self.__add_used_file_path_if_possible(gun, "Weapon", root_directory)



	def __read_object_graphics(self, model_reference, root_directory: str= ""):
		if not href_file_exists(model_reference, self.file_system, root_directory):
			if model_reference.attrib["href"]:
				self.logger.print(f"Visual object file '{model_reference.attrib['href']}' not found! "
								  f"root_directory: '{root_directory}'")
			return
		graphic_xml = href_read_xml_object(model_reference, self.file_system, root_directory)
		root_directory = self.__update_root_directory(model_reference, root_directory)
		for item in graphic_xml.Models.Item:
			self.__read_model(item.Model, root_directory, item.Season.text)
		self.__add_used_file_path(model_reference, root_directory)


	def __read_model(self, vis_obj_reference, root_directory: str="", season:str=None):
		if not href_file_exists(vis_obj_reference, self.file_system, root_directory):
			self.logger.print(f"Model file '{vis_obj_reference.attrib['href']}' not found! "
							  f"root_directory: '{root_directory}', season={season}")
			return
		model_xml = href_read_xml_object(vis_obj_reference, self.file_system, root_directory)
		root_directory = self.__update_root_directory(vis_obj_reference, root_directory)
		for item in model_xml.Materials.Item:
			self.__read_material(item, root_directory)
			self.__add_used_file_path(item, root_directory)
		self.__read_geometry(model_xml.Geometry, root_directory)
		self.__read_skeleton(model_xml.Skeleton, root_directory)

		self.__add_used_file_path(model_xml.Skeleton, root_directory)
		self.__add_used_file_path(model_xml.Geometry, root_directory)
		self.__add_used_file_path(vis_obj_reference, root_directory)

	def __read_material(self, material_reference, root_directory: str= ""):
		if not href_file_exists(material_reference, self.file_system, root_directory):
			self.logger.print(f"Material file '{material_reference.attrib['href']}' not found! "
							  f"root_directory: '{root_directory}'")
			return
		material_xml = href_read_xml_object(material_reference, self.file_system, root_directory)
		root_directory = self.__update_root_directory(material_reference, root_directory)
		self.__read_texture(material_xml.Texture, root_directory)
		self.__add_used_file_path(material_reference, root_directory)

	def __read_texture(self, texture_reference, root_directory: str= ""):
		if not href_file_exists(texture_reference, self.file_system, root_directory):
			self.logger.print(f"Texture file '{texture_reference.attrib['href']}' not found! "
							  f"root_directory: '{root_directory}'")
			return
		texture_xml = href_read_xml_object(texture_reference, self.file_system, root_directory)
		root_directory = self.__update_root_directory(texture_reference, root_directory)
		self.result.add(actual_href_path(texture_xml.DestName, self.file_system, root_directory))
		self.__add_used_file_path(texture_reference, root_directory)

	def __read_geometry(self, geometry_reference, root_directory: str= ""):
		if not href_file_exists(geometry_reference, self.file_system, root_directory):
			self.logger.print(f"Geometry file '{geometry_reference.attrib['href']}' not found! "
							  f"root_directory: '{root_directory}'")
			return
		geometry_xml = href_read_xml_object(geometry_reference, self.file_system, root_directory)
		self.result.add(f"bin/geometries/{geometry_xml.uid}")
		self.__add_used_file_path(geometry_reference, root_directory)
		root_directory = self.__update_root_directory(geometry_reference, root_directory)
		if not href_file_exists(geometry_xml.AIGeometry, self.file_system, root_directory):
			self.logger.print(f"AIGeometry file '{geometry_xml.AIGeometry.attrib['href']}' not found! "
							  f"root_directory: '{root_directory}'")
			return
		ai_geometry = href_read_xml_object(geometry_xml.AIGeometry, self.file_system, root_directory)
		self.result.add(f"bin/aigeometries/{ai_geometry.uid}")
		self.__add_used_file_path(geometry_xml.AIGeometry, root_directory)


	def __read_skeleton(self, skeleton_reference, root_directory: str= ""):
		if not href_file_exists(skeleton_reference, self.file_system, root_directory):
			self.logger.print(f"Skeleton file '{skeleton_reference.attrib['href']}' not found! "
							  f"root_directory: '{root_directory}'")
			return
		skeleton_xml = href_read_xml_object(skeleton_reference, self.file_system, root_directory)
		root_directory = self.__update_root_directory(skeleton_reference, root_directory)
		self.result.add(f"bin/skeletons/{skeleton_xml.uid}")
		if hasattr(skeleton_xml.Animations, "Item"):
			for item in skeleton_xml.Animations.Item:
				if not href_file_exists(item, self.file_system, root_directory):
					self.logger.print(f"Animation file '{item.attrib['href']}' not found! "
									  f"root_directory: '{root_directory}'")
					continue
				animation_xml = href_read_xml_object(item, self.file_system, root_directory)
				self.result.add(f"bin/animations/{animation_xml.uid}")
				self.__add_used_file_path(item, root_directory)
		self.__add_used_file_path(skeleton_reference, root_directory)
