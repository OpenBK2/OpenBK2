from logger_abstract import LoggerAbstract


class ConsoleLogger(LoggerAbstract):

	def __init__(self, *args):
		pass

	def print(self, content):
		print(content)
