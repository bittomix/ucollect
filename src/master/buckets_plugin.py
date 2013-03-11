from twisted.internet.task import LoopingCall
import time
import struct
import plugin


class BucketsPlugin(plugin.Plugin):
	"""
	Counterpart of the "buckets" plugin in the client. It does
	analysis by hashing data into buckets by several statistics.
	"""
	def __init__(self, plugins):
		plugin.Plugin.__init__(self, plugins)
		self.__bucket_count = 16
		self.__hash_count = 4
		self.__criteria = ['I']
		self.__history_size = 1
		self.__config_version = 1
		self.__max_key_count = 102400
		# Just an arbitrary number
		self.__seed = 872945724987
		self.__downloader = LoopingCall(self.__init_download)
		# FIXME: Adjust the time to something reasonable after the testing.
		self.__downloader.start(5, False)

	def name(self):
		return "Buckets"

	def message_from_client(self, message, client):
		kind = message[0]
		if kind == 'C':
			# It asks for config. Send some.
			self.send('C' + self.__config(), client)
		else:
			print("Unkown data from Buckets plugin: " + message)

	def __config(self):
		header = struct.pack('!2Q6L' + str(len(self.__criteria)) + 'c', self.__seed, int(time.time()), self.__bucket_count, self.__hash_count, len(self.__criteria), self.__history_size , self.__config_version, self.__max_key_count, *self.__criteria)
		return header

	def __init_download(self):
		"""
		Ask the clients to provide some data.
		"""
		data = struct.pack('Q', int(time.time()))
		self.broadcast('G' + data)
