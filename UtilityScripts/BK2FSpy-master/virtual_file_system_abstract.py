import typing
import io
import datetime
from abc import ABC, abstractmethod


class VirtualFileSystemBaseClass(ABC):

    @abstractmethod
    def __init__(self, *args):
        pass

    @abstractmethod
    def last_modify_time(self, path: str) -> datetime.datetime:
        pass

    @abstractmethod
    def contains_file(self, path: str) -> bool:
        pass

    @abstractmethod
    def __contains__(self, item):
        pass

    @abstractmethod
    def open_file(self, item):
        pass

    @abstractmethod
    def read_text_file(self, path: str):
        pass

    @abstractmethod
    def read_file_bytes(self, path: str) -> bytes:
        pass

    @abstractmethod
    def write_to_file(self, path: str, content: typing.Union[str, bytes, bytearray]):
        pass

    @abstractmethod
    # used to get the source .pak file
    def get_source_file(self, path: str) -> str:
        pass

    @abstractmethod
    def get_all_files(self) -> list[str]:
        pass
