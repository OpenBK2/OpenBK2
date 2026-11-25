import datetime
import io
import os
import typing
from utils import full_path, formatted_path, decode_bytes_string
from datetime import datetime
from pathlib import Path
from virtual_file_system_abstract import VirtualFileSystemBaseClass


class FolderSystem(VirtualFileSystemBaseClass):

	def __init__(self, *args):
		default_dir = args[0]
		self.default_directory = full_path(default_dir)
		self.directories = []
		self.directories.append(self.default_directory)

	def add_directory(self, new_directory: str):
		self.directories.append(full_path(new_directory))

	def get_newest_file(self, path: str) -> (str, datetime):
		file = None
		file_mtime = None
		for directory in self.directories:
			directory_path = full_path(os.path.join(directory, path))
			if os.path.exists(directory_path):
				if file is None:
					file = directory_path
					file_mtime = os.path.getmtime(directory_path)
					continue
				file_new_mtime = os.path.getmtime(directory_path)
				if file_mtime < file_new_mtime:
					file, file_mtime = directory_path, file_new_mtime
		return file, datetime.fromtimestamp(file_mtime)

	def contains_file(self, path: str) -> bool:
		for directory in self.directories:
			directory_path = full_path(os.path.join(directory, path))
			if os.path.exists(directory_path):
				return True
		return False

	def __contains__(self, item):
		return self.contains_file(item)

	def last_modify_time(self, path: str) -> datetime:
		return self.get_newest_file(path)[1]

	def get_all_files(self) -> list[str]:
		ret = []
		for directory in self.directories:
			for root, dirs, files in os.walk(directory):
				for file in files:
					file_path = os.path.join(root, file)
					file_path = Path(file_path).relative_to(directory if type(directory) is Path else Path(directory))
					file_path = str(file_path)
					ret.append(formatted_path(file_path))
		return ret

	def get_source_file(self, path: str) -> str:
		return "folder: " + os.path.abspath(path)

	def open_file(self, path):
		file, date = self.get_newest_file(path)
		if file is None:
			return None
		return open(file, "rb")

	def read_text_file(self, path: str) -> str:
		with self.open_file(path) as file:
			content = decode_bytes_string(file.read())
			return content

	def read_file_bytes(self, path: str) -> bytes:
		with self.open_file(path) as file:
			return file.read()

	def write_to_file(self, path: str, content: typing.Union[str, bytes, bytearray]):
		path = os.path.join(self.default_directory, path)
		args = {"file": path, "mode": "w", "encoding": "utf-16-le", "newline": ""}
		if isinstance(content, bytes) or isinstance(content, bytearray):
			args["mode"] = "wb"
			args.pop("encoding")
			args.pop("newline")
		with open(**args) as file:
			file.write(content)
