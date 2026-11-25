import logger_abstract


class SetLogger(logger_abstract.LoggerAbstract):

	def __init__(self):
		# use dict instead of set since it preserver the insertion order
		self.messages: dict[str] = dict[str]()

	def print(self, content: str):
		self.messages[content] = 1
