import datetime
import typing
from folder_system import VirtualFileSystemBaseClass
from zipfile import ZipFile, ZipInfo
import glob
from utils import is_slash, decode_bytes_string, formatted_path


class ZipEntry:

	def __init__(self, info: ZipInfo):
		self.info = info
		self.filename = info.filename
		self.date_time = info.date_time


class PakLoader(VirtualFileSystemBaseClass):

	def __init__(self, *args):
		start_dir = args[0]
		logger = args[1]
		self.start_directory = start_dir
		self.logger = logger
		self.paks = []
		self.zip_entries = dict()
		self.open_directory(start_dir)

	def __path_format(self, path: str) -> str:
		return formatted_path(path)

	def open_directory(self, directory):
		path = directory + "/*.pak"
		if is_slash(directory[-1]):
			path = directory + "*.pak"
		files = glob.glob(path)
		files = reversed(sorted(files))
		for file in files:
			try:
				archive = ZipFile(file, "r")
				self.paks.append(archive)
				for entry_iter in archive.filelist:
					entry = ZipEntry(entry_iter)
					if is_slash(entry.filename[-1]):
						continue
					entry_path = self.__path_format(entry.filename)
					edit_time = datetime.datetime(*entry.date_time)
					entry.archive = archive
					if entry_path in self.zip_entries.keys():
						other_edit_time = datetime.datetime(*self.zip_entries[entry_path].date_time)
						if edit_time > other_edit_time:
							self.zip_entries[entry_path] = entry
					else:
						self.zip_entries[entry_path] = entry
			except Exception as e:
				self.logger.print(e)
				pass
		pass

	def contains_file(self, path: str) -> bool:
		return self.__path_format(path) in self.zip_entries

	def last_modify_time(self, path: str) -> datetime.datetime:
		return datetime.datetime(*self.zip_entries[self.__path_format(path)].date_time)

	def __contains__(self, item):
		return self.contains_file(item)

	def open_file(self, item):
		path = self.__path_format(item)
		if self.contains_file(path):
			entry = self.zip_entries[path]
			return entry.archive.open(entry.filename, "r")
		return None

	def read_file_bytes(self, path: str) -> bytes:
		return self.open_file(path).read()

	def read_text_file(self, path: str):
		return decode_bytes_string(self.read_file_bytes(path))

	def write_to_file(self, path: str, content: typing.Union[str, bytes, bytearray]):
		raise Exception("Cannot write files in .pak archives")

	def get_source_file(self, path: str) -> str:
		path = self.__path_format(path)
		return "pak: " + self.zip_entries[path].archive.filename

	def get_all_files(self) -> list[str]:
		ret = []
		for val in self.zip_entries.values():
			ret.append(formatted_path(val.filename))
		return ret
