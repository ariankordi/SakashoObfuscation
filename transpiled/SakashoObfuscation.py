# Generated automatically with "fut". Do not edit.
import array

class Varint:
	"""Reads and writes Base128 Varints used in Protobufs, DWARF..."""

	MAX_SIZE_INT = 5
	"""Maximum size for a 32-bit integer as a varint."""

	@staticmethod
	def read(data: bytearray | bytes, pos_out: bytearray) -> int:
		"""Reads a varint from data starting at posOut[0].

		Advances posOut[0] to indicate how large the varint was.
		Maximum is a 32-bit int. Returns 0 on invalid varint.

		:param data: Byte array containing the varint.
		:param pos_out: Single-element array containing the cursor.
		"""
		value: int = 0
		shift: int = 0
		pos: int = pos_out[0]
		for _ in range(5):
			b: int = data[pos]
			pos += 1
			value |= (b & 127) << shift
			if (b & 128) == 0:
				pos_out[0] = pos
				return value
			shift += 7
			if shift >= 35:
				return 0
		return 0

	@staticmethod
	def write(dst: bytearray, value: int, offset: int = 0) -> int:
		"""Writes a 32-bit varint to the specified output.

		Returns varint size, or 0 on failure.

		:param offset: Offset within dst to write the varint to.
		"""
		pos: int = 0
		for _ in range(5):
			b: int = value & 127
			value >>= 7
			if value != 0:
				dst[offset + pos] = b | 128
				pos += 1
			else:
				dst[offset + pos] = b
				pos += 1
				return pos
		return 0

class Lz4:
	"""Simple LZ4 en/decoder. Goals are to be small and safe.

	Does not use the LZ4 frame format, and does not perform compression."""

	@staticmethod
	def decompress(src: bytearray | bytes, dst: bytearray, compressed_size: int, dst_capacity: int, src_offset: int = 0) -> int:
		"""Decompresses a chunk of raw LZ4 data into the output buffer.

		Returns the number of bytes decompressed, or -1 on corruption.

		:param src: Compressed input data.
		:param dst: Decompressed output.
		:param compressed_size: Length of compressed input data.
		:param dst_capacity: Capacity of the output buffer.
		:param src_offset: Offset into the compressed data.
		"""
		src_pos: int = 0
		dst_pos: int = 0
		while src_pos < compressed_size and dst_pos < dst_capacity:
			if src_pos >= compressed_size:
				return -1
			token: int = src[src_offset + src_pos]
			src_pos += 1
			enc_count: int = token & 15
			lit_count: int = token >> 4 & 15
			if lit_count == 15:
				sum: int = 0
				while True:
					if src_pos >= compressed_size:
						return -1
					sum = src[src_offset + src_pos]
					src_pos += 1
					lit_count += sum
					if not (sum == 255):
						break
			if src_pos + lit_count > compressed_size:
				return -1
			if dst_pos + lit_count > dst_capacity:
				return -1
			dst[dst_pos:dst_pos + lit_count] = src[src_offset + src_pos:src_offset + src_pos + lit_count]
			src_pos += lit_count
			dst_pos += lit_count
			if src_pos >= compressed_size:
				break
			if src_pos + 1 >= compressed_size:
				return -1
			back: int = src[src_offset + src_pos] | src[src_offset + src_pos + 1] << 8
			src_pos += 2
			if back <= 0 or back > dst_pos:
				return -1
			if enc_count == 15:
				sum: int = 0
				while True:
					if src_pos >= compressed_size:
						return -1
					sum = src[src_offset + src_pos]
					src_pos += 1
					enc_count += sum
					if not (sum == 255):
						break
			enc_count += 4
			if dst_pos + enc_count > dst_capacity:
				return -1
			enc_pos: int = dst_pos - back
			if enc_count <= back:
				dst[dst_pos:dst_pos + enc_count] = dst[enc_pos:enc_pos + enc_count]
				dst_pos += enc_count
			else:
				for _ in range(enc_count):
					dst[dst_pos] = dst[enc_pos]
					dst_pos += 1
					enc_pos += 1
		return dst_pos

	@staticmethod
	def get_max_compressed_size(input_size: int) -> int:
		"""Gets the maximum size of a compressed buffer, same as LZ4_compressBound."""
		return 0 if input_size > 2113929216 else input_size + int(input_size / 255) + 16

	@staticmethod
	def compress(src: bytearray | bytes, dst: bytearray, src_size: int, dst_capacity: int, dst_offset: int = 0) -> int:
		"""Encodes input into raw LZ4 block format with no framing.

		Returns the number of bytes written, or -1 on failure.
		This does not actually compress the data, leaving it
		larger than it came in just for interoperability.

		:param src: Input data.
		:param dst: Destination to write compressed data to.
		:param src_size: Size of input data buffer.
		:param dst_capacity: Capacity of the output buffer.
		:param dst_offset: Offset into the destination buffer.
		"""
		if src_size < 0:
			return -1
		dst_pos: int = 0
		lit_len: int = src_size
		token_lit: int = lit_len if lit_len < 15 else 15
		if dst_pos >= dst_capacity:
			return -1
		dst[dst_offset + dst_pos] = token_lit << 4
		dst_pos += 1
		if lit_len >= 15:
			len_: int = lit_len - 15
			while len_ >= 255:
				if dst_pos >= dst_capacity:
					return -1
				dst[dst_offset + dst_pos] = 255
				dst_pos += 1
				len_ -= 255
			if dst_pos >= dst_capacity:
				return -1
			dst[dst_offset + dst_pos] = len_
			dst_pos += 1
		if dst_pos + src_size > dst_capacity:
			return -1
		dst[dst_offset + dst_pos:dst_offset + dst_pos + src_size] = src[0:src_size]
		dst_pos += src_size
		return dst_pos

class SakashoObfuscation:
	"""Deobfuscation for DeNA Sakasho HTTP request/response content.

	Reverse engineered from Miitomo, may be used elsewhere.
	They call this "CookedResponse"/"CookedRequestBody" in symbols.
	Consists of XOR/bit rotation, LZ4 compression, and varint length field.
	This class just implements the XOR logic and a generic decode method
	calling the Varint and Lz4 classes implemented here."""

	def __init__(self):
		self._xor_table = bytearray(256)
	_xor_table: bytearray
	_xor_len: int

	def initialize(self, common_key: str, session_id: str) -> None:
		"""Builds or rebuilds the internal XOR table.

		:param common_key: The common key string.
		:param session_id: The value of the player_session_id cookie,
		or an empty string if the cookie was not set.
		"""
		self._xor_len = 0
		i: int = 0
		while i < len(common_key) and self._xor_len < 256:
			c: int = ord(common_key[i])
			self._xor_table[self._xor_len] = (-98 - c) & 255
			self._xor_len += 1
			i += 1
		i: int = 0
		while i < len(session_id) and self._xor_len < 256:
			self._xor_table[self._xor_len] = ord(session_id[i])
			self._xor_len += 1
			i += 1

	def xor_decode(self, data: bytearray, data_len: int) -> None:
		"""Applies XOR decoding to the buffer in-place."""
		SakashoObfuscation.xor_decode_buffer(data, data_len, self._xor_table, self._xor_len)

	def xor_encode(self, data: bytearray, data_len: int) -> None:
		"""Applies XOR encoding to the buffer in-place."""
		SakashoObfuscation.xor_encode_buffer(data, data_len, self._xor_table, self._xor_len)

	def get_decompressed_size(self, data: bytearray | bytes, pos_out: bytearray | None = None) -> int:
		"""Gets decompressed size from obfuscated/compressed
		data, or 0 if the size varint is invalid."""
		tmp: bytearray = bytearray(5)
		tmp[0:5] = data[0:5]
		self.xor_decode(tmp, 5)
		pos_out_local: bytearray = bytearray(1)
		pos_out_local[0] = 0
		return Varint.read(tmp, pos_out_local) if pos_out is None else Varint.read(tmp, pos_out)

	def decode(self, data: bytearray | bytes, data_len: int) -> bytearray | None:
		"""Fully decodes and decompresses obfuscated bytes.

		Returns a pre-allocated byte array (must be freed by the caller)
		or null if the decompression failed."""
		tmp: bytearray = bytearray(data_len)
		tmp[0:data_len] = data[0:data_len]
		self.xor_decode(tmp, data_len)
		pos_out: bytearray = bytearray(1)
		pos_out[0] = 0
		size: int = Varint.read(tmp, pos_out)
		if size <= 0:
			return None
		assert size <= 104857600
		output: bytearray = bytearray(size)
		decompressed: int = Lz4.decompress(tmp, output, data_len - pos_out[0], size, pos_out[0])
		if decompressed <= 0:
			return None
		return output

	def encode(self, data: bytearray | bytes, data_len: int, pos_out: array.array) -> bytearray | None:
		"""Fully compresses and obfuscates raw bytes.

		Returned a pre-allocated byte array (must be freed by the caller)
		containing the compressed and obfuscated data, or null on failure.
		NOTE: The length of the array is posOut. You must trim the output.
		Example: const len = new Uint8Array([0]);
		let result = obfs.encode(in, size, len); result = result.subarray(0, len);

		:param pos_out: Array where the 0th element is the output size.
		"""
		max_compressed: int = Lz4.get_max_compressed_size(data_len)
		if max_compressed <= 0:
			return None
		buffer: bytearray = bytearray(5 + max_compressed)
		varint_size: int = Varint.write(buffer, data_len, 0)
		if varint_size <= 0:
			return None
		compressed_size: int = Lz4.compress(data, buffer, data_len, max_compressed, varint_size)
		if compressed_size <= 0:
			return None
		total_size: int = varint_size + compressed_size
		self.xor_encode(buffer, total_size)
		pos_out[0] = total_size
		return buffer

	@staticmethod
	def xor_decode_buffer(data: bytearray, data_len: int, table: bytearray | bytes, table_len: int) -> None:
		"""Applies the conditional XOR/bit rotation operation to decode the data.

		Reference: libsakasho.so:FUN_0004ec70, Java_jp_dena_sakasho_core_delegate_CookedResponseDelegate_cookResponse"""
		for i in range(data_len):
			key_byte: int = table[(i + 1) % table_len]
			input_byte: int = data[i]
			if (key_byte & 7) == 0:
				data[i] = input_byte ^ key_byte
			else:
				shift: int = key_byte & 7
				data[i] = (input_byte >> (8 - shift) | input_byte << shift) & 255

	@staticmethod
	def xor_encode_buffer(data: bytearray, data_len: int, table: bytearray | bytes, table_len: int) -> None:
		"""Applies the conditional XOR/bit rotation operation to encode the data.

		Reference: libsakasho.so:FUN_0004ebc0, Java_jp_dena_sakasho_core_http_CookedRequestBody_cookRequest"""
		for i in range(data_len):
			key_byte: int = table[(i + 1) % table_len]
			input_byte: int = data[i]
			if (key_byte & 7) == 0:
				data[i] = input_byte ^ key_byte
			else:
				shift: int = key_byte & 7
				data[i] = (input_byte << (8 - shift) | input_byte >> shift) & 255
