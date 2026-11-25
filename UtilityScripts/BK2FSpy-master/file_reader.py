from folder_system import FolderSystem
from pak_loader import PakLoader
from console_logger import ConsoleLogger
from virtual_file_system import VirtualFileSystem
from enum import Enum
import shlex


class Modes(Enum):
	EXIT = 0
	PRINT_TO_CONSOLE = 1
	WRITE_TO_FILE = 2
	FILE_EXISTS = 3


game_path = r"C:\Program Files (x86)\Blitzkrieg 2 -  Total Conversion\Data"


def exit_app(*args):
	exit(0)


def print_to_console(*args):
	fs: VirtualFileSystem = args[0]
	path: str = args[1]
	print(fs.read_text_file(path))


def write_to_file(*args):
	fs: VirtualFileSystem = args[0]
	path: str = args[1]
	path_out = args[2]
	with open(path_out, "wb") as file:
		file.write(fs.read_file_bytes(path))


def file_exists(fs: VirtualFileSystem, path: str):
	print(fs.contains_file(path))


modes_dict = {
	Modes.EXIT: exit_app,
	Modes.PRINT_TO_CONSOLE: print_to_console,
	Modes.WRITE_TO_FILE: write_to_file,
	Modes.FILE_EXISTS: file_exists
	}

modes_keys = {
	"p": Modes.PRINT_TO_CONSOLE,
	"print": Modes.PRINT_TO_CONSOLE,
	"w": Modes.WRITE_TO_FILE,
	"write": Modes.WRITE_TO_FILE,
	"c": Modes.FILE_EXISTS,
	"check": Modes.FILE_EXISTS,
	"fe": Modes.FILE_EXISTS,
	"exit": Modes.EXIT,
}


def main():
	global game_path

	logger = ConsoleLogger()
	folder = FolderSystem(game_path)
	fs = VirtualFileSystem(folder)
	pk = PakLoader(game_path, logger)
	fs.add_system(pk)

	while 1:
		args = input("Enter command: ")
		args = shlex.split(args)
		mode = modes_keys[args[0]]
		args[0] = fs
		args = tuple(args)
		modes_dict[mode](*args)
	return 0


if __name__ == "__main__":
	main()
