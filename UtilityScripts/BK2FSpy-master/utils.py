import os
import codecs


def full_path(path: str):
	return os.path.abspath(path).lower().replace("\\", "/")


def formatted_path(path: str):
	return path.lower().replace("\\", "/")


def path_folder(path: str):
	return os.path.dirname(formatted_path(path))


def decode_bytes_string(string: bytes):
	encoding = "utf-8"
	# UTF-16 LE BOM detection
	if string[0] == 255 and string[1] == 254:
		encoding = "utf-16-le"
	return string.decode(encoding)


def string_to_utf16_le(string: str):
	return codecs.BOM_UTF16_LE + str.encode(string, "utf-16-le")


def is_slash(string: str):
	return string == "/" or string == "\\"


def tuple_sum(a: tuple, b: tuple):
	count = max(len(a), len(b))
	ret = [0 for i in range(count)]
	for i in range(count):
		tmp_sum = 0
		if i < len(a):
			tmp_sum += a[i]
		if i < len(b):
			tmp_sum += b[i]
		ret[i] = tmp_sum
	return tuple(ret)


def tuple_add(a: tuple, b: tuple):
	return tuple([a[i] + b[i] for i in range(len(a))])


def tuple_sub(a: tuple, b: tuple):
	return tuple([a[i] - b[i] for i in range(len(a))])


def tuple_scalar_multiply(a: tuple, b: float):
	ret = list(a)
	for i in range(len(ret)):
		ret[i] *= b
	return tuple(ret)


def tuple_average(tuples: list):
	result = tuples[0]
	for i in range(1, len(tuples)):
		result = tuple_sum(result, tuples[i])
	result = tuple_scalar_multiply(result, 1.0 / len(tuples))
	return result
