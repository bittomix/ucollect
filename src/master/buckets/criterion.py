#
#    Ucollect - small utility for real-time analysis of network data
#    Copyright (C) 2013 CZ.NIC, z.s.p.o. (http://www.nic.cz/)
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License along
#    with this program; if not, write to the Free Software Foundation, Inc.,
#    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#

import struct
import socket

class Criterion:
	"""
	One criterion to hash by.

	This is the (abstract) base class. Use concrete derived classes.
	"""
	def decode_multiple(self, data):
		"""
		Decode multiple data concatenated together. Returns list of the strings.
		"""
		return map(self.decode, self.decode_raw_multiple(data))

	def decode_raw_multiple(self, data):
		"""
		Extract the data into sequence of binary representations of the keys,
		as they are on the wire.
		"""
		l = self.item_len()
		result = []
		while data:
			result.append(data[:l])
			data = data[l:]
		return result

class Port(Criterion):
	"""
	A remote port of TCP/UDP packet. Encoded as 2 bytes in network byte order.
	"""
	def item_len(self):
		"""
		Length of single item.
		"""
		return 2

	def code(self):
		"""
		The on-wire code of this criterion.
		"""
		return 'P'

	def decode(self, data):
		"""
		Decode port on wire to string.
		"""
		(port,) = struct.unpack('!H', data)
		return str(port)

class Address(Criterion):
	"""
	An IPv4/6 address of the remote endpoint of communication.

	Always encoded as 17 bytes (version byte and then 16 bytes, IPv4 just wastes).
	"""
	def item_len(self):
		"""
		Length of single item.
		"""
		return 17

	def code(self):
		"""
		The on-wire code of this criterion.
		"""
		return 'I'

	def decode(self, data):
		"""
		Decode the IP address on wire to string.
		"""
		if data[0] == '\x04': # IPv4
			return socket.inet_ntop(socket.AF_INET, data[1:5])
		elif data[0] == '\x06': # IPv6
			return socket.inet_ntop(socket.AF_INET6, data[1:])
		else:
			raise ValueError('Unknown IP address version')

class CompoundCriterion(Criterion):
	"""
	A criterion built of list of other criteria.

	They are concatenated together in the wire format. The parsed
	strings are joined together by designated string (separator).
	"""
	def __init__(self, parts, separator):
		self.__parts = parts
		self.__separator = separator

	def item_len(self):
		"""
		Length of single item.
		"""
		return sum(map(lambda p: p.item_len(), self.__parts))

	def decote(self, data):
		"""
		Decode the data to string representation.
		"""
		return self.__separator.join(self.decode_parts(data))

	def decode_parts(self, data):
		"""
		Decode the data to list of strings, one for each part.
		"""
		result = []
		for p in self.__parts:
			l = p.item_len()
			result.append(p.decode(data[:l]))
			data = data[l:]
		return result

class AddressAndPort(CompoundCriterion):
	"""
	Just port followed by an address.
	"""
	def __init__(self):
		CompoundCriterion.__init__(self, [Port(), Address()], ':')

	def code(self):
		"""
		The on-wire code of this criterion.
		"""
		return 'B'

	def decode(self, data):
		"""
		Decode data to string representation.

		We use custom formatting of the parts together, since they are in reverse order than
		what we want. Also, IPv6 addresses are usually put to square brackets.
		"""
		(port, ip) = self.decode_parts(data)
		if ':' in ip:
			ip = '[' + ip + ']'
		return ':'.join([ip, port])

class AddressAndLocalPort(CompoundCriterion):
	"""
	Just port followed by an address, but with different syntax.
	"""
	def __init__(self):
		CompoundCriterion.__init__(self, [Port(), Address()], '->')

	def code(self):
		"""
		The on-wire code of this criterion.
		"""
		return 'L'

	def decode(self, data):
		"""
		Decode data to string representation.

		We use custom formatting of the parts together, since they are in reverse order than
		what we want. Also, IPv6 addresses are usually put to square brackets.
		"""
		(port, ip) = self.decode_parts(data)
		if ':' in ip:
			ip = '[' + ip + ']'
		return '->'.join([ip, port])

class PortOut(Port):
	"""
	Port of outbound packets.
	"""
	def code(self):
		return 'p'

class AddressOut(Address):
	"""
	Adress of outbound packets.
	"""
	def code(self):
		return 'i'

class AddressAndPortOut(AddressAndPort):
	"""
	Address and port of outbound packets.
	"""
	def code(self):
		return 'b'

class AddressAndLocalPortOut(AddressAndLocalPort):
	"""
	Remote address and local port of outbound packets. It is probably useless and is supported just for completeness.
	"""
	def code(self):
		return 'l'
