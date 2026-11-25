import datetime
import io
import typing

import utils
from utils import decode_bytes_string
from virtual_file_system_abstract import VirtualFileSystemBaseClass


class VirtualFileSystem(VirtualFileSystemBaseClass):

	def __init__(self, *args):
		default_system = args[0]
		self.default_system: VirtualFileSystemBaseClass = default_system
		self.cache: dict[str, bytes] = dict()
		self.cache_time: dict[str, datetime.datetime] = dict()
		self.systems: list[VirtualFileSystemBaseClass] = [default_system]

	def add_system(self, system: VirtualFileSystemBaseClass):
		self.systems.append(system)

	def contains_file(self, path: str) -> bool:
		if not path.strip():
			return False
		if path in self.cache:
			return True
		for system in self.systems:
			if system.contains_file(path):
				return True
		return False

	def last_modify_time(self, path: str) -> datetime.datetime:
		if path in self.cache_time:
			return self.cache_time[path]
		if not self.contains_file(path):
			raise Exception(f"File: \"{path}\" not found!")
		ret = None
		for system in self.systems:
			if system.contains_file(path):
				if ret is None:
					ret = system.last_modify_time(path)
					continue
				modify_time = system.last_modify_time(path)
				if ret < modify_time:
					ret = modify_time
		if ret is not None:
			self.cache_time[path] = ret
		return ret

	def __contains__(self, item):
		return self.contains_file(item)

	def open_file(self, item):
		item = utils.formatted_path(item)
		if item in self.cache:
			return io.BytesIO(self.cache[item])
		ret = None
		time = None
		for system in self.systems:
			if system.contains_file(item):
				t = system.last_modify_time(item)
				if ret is None:
					ret = system.open_file(item)
					time = t
				if time < t:
					ret.close()
					ret = system.open_file(item)
					time = t
		if ret is not None:
			self.cache[item] = ret.read()
			ret.close()
			ret = io.BytesIO(self.cache[item])
		return ret

	def read_file_bytes(self, path: str) -> bytes:
		with self.open_file(path) as stream:
			return stream.read()

	def read_text_file(self, path: str):
		return decode_bytes_string(self.read_file_bytes(path))

	def write_to_file(self, path: str, content: typing.Union[str, bytes, bytearray]):
		self.default_system.write_to_file(path, content)

	def get_source_file(self, path: str) -> str:
		ret = None
		time = None
		for system in self.systems:
			if system.contains_file(path):
				t = system.last_modify_time(path)
				if ret is None:
					ret = system.get_source_file(path)
					time = t
					continue
				if time < t:
					ret = system.get_source_file(path)
					time = t
		return ret

	def get_all_files(self) -> list[str]:
		ret = []
		for system in self.systems:
			ret.extend(system.get_all_files())
		return list(set(ret))
