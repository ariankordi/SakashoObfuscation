// Generated automatically with "fut". Do not edit.
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "SakashoObfuscation.h"

typedef void (*FuMethodPtr)(void *);
typedef struct {
	size_t count;
	size_t unitSize;
	size_t refCount;
	FuMethodPtr destructor;
} FuShared;

static void *FuShared_Make(size_t count, size_t unitSize, FuMethodPtr constructor, FuMethodPtr destructor)
{
	FuShared *self = (FuShared *) malloc(sizeof(FuShared) + count * unitSize);
	self->count = count;
	self->unitSize = unitSize;
	self->refCount = 1;
	self->destructor = destructor;
	if (constructor != NULL) {
		for (size_t i = 0; i < count; i++)
			constructor((char *) (self + 1) + i * unitSize);
	}
	return self + 1;
}

static void FuShared_Release(void *ptr)
{
	if (ptr == NULL)
		return;
	FuShared *self = (FuShared *) ptr - 1;
	if (--self->refCount != 0)
		return;
	if (self->destructor != NULL) {
		for (size_t i = self->count; i > 0;)
			self->destructor((char *) ptr + --i * self->unitSize);
	}
	free(self);
}

/**
 * Deobfuscation for DeNA Sakasho HTTP request/response content.
 * 
 * <p>Reverse engineered from Miitomo, may be used elsewhere.
 * They call this "CookedResponse"/"CookedRequestBody" in symbols.
 * <p>Consists of XOR/bit rotation, LZ4 compression, and varint length field.
 * This class just implements the XOR logic and a generic decode method
 * calling the Varint and Lz4 classes implemented here.
 */
struct SakashoObfuscation {
	uint8_t xorTable[256];
	int xorLen;
};

/**
 * Adds bytes to the XOR table, making sure to not overflow it.
 * Typically the transformed common key is added, and then the session ID.
 * @param self This <code>SakashoObfuscation</code>.
 */
static void SakashoObfuscation_AddToXorTable(SakashoObfuscation *self, uint8_t const *b, int length);

int Varint_Read(uint8_t const *data, uint8_t *posOut)
{
	int value = 0;
	int shift = 0;
	int pos = posOut[0];
	for (int i = 0; i < 5; i++) {
		int b = data[pos++];
		value |= (b & 127) << shift;
		if ((b & 128) == 0) {
			posOut[0] = (uint8_t) pos;
			return value;
		}
		shift += 7;
		if (shift >= 35)
			return 0;
	}
	return 0;
}

int Varint_Write(uint8_t *dst, int value, int offset)
{
	int pos = 0;
	for (int i = 0; i < 5; i++) {
		int b = value & 127;
		value >>= 7;
		if (value != 0) {
			dst[offset + pos++] = (uint8_t) (b | 128);
		}
		else {
			dst[offset + pos++] = (uint8_t) b;
			return pos;
		}
	}
	return 0;
}

int Lz4_Decompress(uint8_t const *src, uint8_t *dst, int compressedSize, int dstCapacity, int srcOffset)
{
	int srcPos = 0;
	int dstPos = 0;
	while (srcPos < compressedSize && dstPos < dstCapacity) {
		if (srcPos >= compressedSize)
			return -1;
		int token = src[srcOffset + srcPos++];
		int encCount = token & 15;
		int litCount = token >> 4 & 15;
		if (litCount == 15) {
			int sum = 0;
			do {
				if (srcPos >= compressedSize)
					return -1;
				sum = src[srcOffset + srcPos++];
				litCount += sum;
			}
			while (sum == 255);
		}
		if (srcPos + litCount > compressedSize)
			return -1;
		if (dstPos + litCount > dstCapacity)
			return -1;
		memcpy(dst + dstPos, src + (srcOffset + srcPos), litCount);
		srcPos += litCount;
		dstPos += litCount;
		if (srcPos >= compressedSize)
			break;
		if (srcPos + 1 >= compressedSize)
			return -1;
		int back = src[srcOffset + srcPos] | src[srcOffset + srcPos + 1] << 8;
		srcPos += 2;
		if (back <= 0 || back > dstPos)
			return -1;
		if (encCount == 15) {
			int sum = 0;
			do {
				if (srcPos >= compressedSize)
					return -1;
				sum = src[srcOffset + srcPos++];
				encCount += sum;
			}
			while (sum == 255);
		}
		encCount += 4;
		if (dstPos + encCount > dstCapacity)
			return -1;
		int encPos = dstPos - back;
		if (encCount <= back) {
			memcpy(dst + dstPos, dst + encPos, encCount);
			dstPos += encCount;
		}
		else {
			for (int i = 0; i < encCount; i++)
				dst[dstPos++] = dst[encPos++];
		}
	}
	return dstPos;
}

int Lz4_GetMaxCompressedSize(int inputSize)
{
	return inputSize > 2113929216 ? 0 : inputSize + inputSize / 255 + 16;
}

int Lz4_Compress(uint8_t const *src, uint8_t *dst, int srcSize, int dstCapacity, int dstOffset)
{
	if (srcSize <= 0)
		return -1;
	int dstPos = 0;
	int litLen = srcSize;
	int tokenLit = litLen < 15 ? litLen : 15;
	if (dstPos >= dstCapacity)
		return -1;
	dst[dstOffset + dstPos++] = (uint8_t) (tokenLit << 4);
	if (litLen >= 15) {
		int len = litLen - 15;
		while (len >= 255) {
			if (dstPos >= dstCapacity)
				return -1;
			dst[dstOffset + dstPos++] = 255;
			len -= 255;
		}
		if (dstPos >= dstCapacity)
			return -1;
		dst[dstOffset + dstPos++] = (uint8_t) len;
	}
	if (dstPos + srcSize > dstCapacity)
		return -1;
	memcpy(dst + (dstOffset + dstPos), src, srcSize);
	dstPos += srcSize;
	return dstPos;
}

void SakashoObfuscation_InitializeMiitomo(SakashoObfuscation *self, const char *playerSessionId)
{
	SakashoObfuscation_InitializeWithKey(self, SakashoObfuscation_COMMON_KEY_MIITOMO, NULL, 0);
	for (int i = 0; i < (ptrdiff_t) strlen(playerSessionId) && self->xorLen < 256; i++)
		self->xorTable[self->xorLen++] = (uint8_t) playerSessionId[i];
}

void SakashoObfuscation_InitializeWithKey(SakashoObfuscation *self, uint8_t const *key, uint8_t const *sessionId, int sessionIdLength)
{
	self->xorLen = 0;
	SakashoObfuscation_AddToXorTable(self, key, 32);
	assert(sessionIdLength <= 0 || sessionId != NULL);
	SakashoObfuscation_AddToXorTable(self, sessionId, sessionIdLength);
}

static void SakashoObfuscation_AddToXorTable(SakashoObfuscation *self, uint8_t const *b, int length)
{
	for (int i = 0; i < length && self->xorLen < 256; i++)
		self->xorTable[self->xorLen++] = b[i];
}

void SakashoObfuscation_XorDecode(const SakashoObfuscation *self, uint8_t *data, int dataLen)
{
	SakashoObfuscation_XorDecodeBuffer(data, dataLen, self->xorTable, self->xorLen);
}

void SakashoObfuscation_XorEncode(const SakashoObfuscation *self, uint8_t *data, int dataLen)
{
	SakashoObfuscation_XorEncodeBuffer(data, dataLen, self->xorTable, self->xorLen);
}

int SakashoObfuscation_GetDecompressedSize(const SakashoObfuscation *self, uint8_t const *data, uint8_t *posOut)
{
	uint8_t tmp[5];
	memcpy(tmp, data, 5);
	SakashoObfuscation_XorDecode(self, tmp, 5);
	uint8_t posOutLocal[1];
	posOutLocal[0] = 0;
	return posOut == NULL ? Varint_Read(tmp, posOutLocal) : Varint_Read(tmp, posOut);
}

uint8_t *SakashoObfuscation_Decode(const SakashoObfuscation *self, uint8_t const *data, int dataLen)
{
	uint8_t *tmp = (uint8_t *) malloc(dataLen * sizeof(uint8_t));
	memcpy(tmp, data, dataLen);
	SakashoObfuscation_XorDecode(self, tmp, dataLen);
	uint8_t posOut[1];
	posOut[0] = 0;
	int size = Varint_Read(tmp, posOut);
	if (size <= 0) {
		free(tmp);
		return NULL;
	}
	assert(size <= 104857600);
	uint8_t *output = (uint8_t *) FuShared_Make(size, sizeof(uint8_t), NULL, NULL);
	int decompressed = Lz4_Decompress(tmp, output, dataLen - posOut[0], size, posOut[0]);
	if (decompressed <= 0) {
		FuShared_Release(output);
		free(tmp);
		return NULL;
	}
	free(tmp);
	return output;
}

uint8_t *SakashoObfuscation_Encode(const SakashoObfuscation *self, uint8_t const *data, int dataLen, int *posOut)
{
	int maxCompressed = Lz4_GetMaxCompressedSize(dataLen);
	if (maxCompressed <= 0)
		return NULL;
	uint8_t *buffer = (uint8_t *) FuShared_Make(5 + maxCompressed, sizeof(uint8_t), NULL, NULL);
	int varintSize = Varint_Write(buffer, dataLen, 0);
	if (varintSize <= 0) {
		FuShared_Release(buffer);
		return NULL;
	}
	int compressedSize = Lz4_Compress(data, buffer, dataLen, maxCompressed, varintSize);
	if (compressedSize <= 0) {
		FuShared_Release(buffer);
		return NULL;
	}
	int totalSize = varintSize + compressedSize;
	SakashoObfuscation_XorEncode(self, buffer, totalSize);
	posOut[0] = totalSize;
	return buffer;
}

void SakashoObfuscation_XformCommonKey(uint8_t *key)
{
	for (int i = 0; i < 32; i++) {
		int c = key[i];
		key[i] = (uint8_t) (-98 - c);
	}
}

void SakashoObfuscation_XorDecodeBuffer(uint8_t *data, int dataLen, uint8_t const *table, int tableLen)
{
	for (int i = 0; i < dataLen; i++) {
		int keyByte = table[(i + 1) % tableLen];
		int inputByte = data[i];
		if ((keyByte & 7) == 0) {
			data[i] = (uint8_t) (inputByte ^ keyByte);
		}
		else {
			int shift = keyByte & 7;
			data[i] = (uint8_t) (inputByte >> (8 - shift) | inputByte << shift);
		}
	}
}

void SakashoObfuscation_XorEncodeBuffer(uint8_t *data, int dataLen, uint8_t const *table, int tableLen)
{
	for (int i = 0; i < dataLen; i++) {
		int keyByte = table[(i + 1) % tableLen];
		int inputByte = data[i];
		if ((keyByte & 7) == 0) {
			data[i] = (uint8_t) (inputByte ^ keyByte);
		}
		else {
			int shift = keyByte & 7;
			data[i] = (uint8_t) (inputByte << (8 - shift) | inputByte >> shift);
		}
	}
}
