// Generated automatically with "fut". Do not edit.
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct SakashoObfuscation SakashoObfuscation;

/**
 * Maximum size for a 32-bit integer as a varint.
 */
#define Varint_MAX_SIZE_INT 5

/**
 * Reads a varint from data starting at posOut[0].
 * Advances posOut[0] to indicate how large the varint was.
 * Maximum is a 32-bit int. Returns 0 on invalid varint.
 * @param data Byte array containing the varint.
 * @param posOut Single-element array containing the cursor.
 */
int Varint_Read(uint8_t const *data, uint8_t *posOut);

/**
 * Writes a 32-bit varint to the specified output.
 * Returns varint size, or 0 on failure.
 * @param offset Offset within dst to write the varint to.
 */
int Varint_Write(uint8_t *dst, int value, int offset);

/**
 * Decompresses a chunk of raw LZ4 data into the output buffer.
 * Returns the number of bytes decompressed, or -1 on corruption.
 * @param src Compressed input data.
 * @param dst Decompressed output.
 * @param compressedSize Length of compressed input data.
 * @param dstCapacity Capacity of the output buffer.
 * @param srcOffset Offset into the compressed data.
 */
int Lz4_Decompress(uint8_t const *src, uint8_t *dst, int compressedSize, int dstCapacity, int srcOffset);

/**
 * Gets the maximum size of a compressed buffer, same as LZ4_compressBound.
 */
int Lz4_GetMaxCompressedSize(int inputSize);

/**
 * Encodes input into raw LZ4 block format with no framing.
 *
 * <p>Returns the number of bytes written, or -1 on failure.
 * <p>This does not actually compress the data, leaving it
 * larger than it came in just for interoperability.
 * @param src Input data.
 * @param dst Destination to write compressed data to.
 * @param srcSize Size of input data buffer.
 * @param dstCapacity Capacity of the output buffer.
 * @param dstOffset Offset into the destination buffer.
 */
int Lz4_Compress(uint8_t const *src, uint8_t *dst, int srcSize, int dstCapacity, int dstOffset);

/**
 * Builds or rebuilds the internal XOR table.
 * @param self This <code>SakashoObfuscation</code>.
 * @param commonKey The common key string.
 * @param sessionId The value of the player_session_id cookie,
 * or an empty string if the cookie was not set.
 */
void SakashoObfuscation_Initialize(SakashoObfuscation *self, const char *commonKey, const char *sessionId);

/**
 * Applies XOR decoding to the buffer in-place.
 * @param self This <code>SakashoObfuscation</code>.
 */
void SakashoObfuscation_XorDecode(const SakashoObfuscation *self, uint8_t *data, int dataLen);

/**
 * Applies XOR encoding to the buffer in-place.
 * @param self This <code>SakashoObfuscation</code>.
 */
void SakashoObfuscation_XorEncode(const SakashoObfuscation *self, uint8_t *data, int dataLen);

/**
 * Gets decompressed size from obfuscated/compressed
 * data, or 0 if the size varint is invalid.
 * @param self This <code>SakashoObfuscation</code>.
 */
int SakashoObfuscation_GetDecompressedSize(const SakashoObfuscation *self, uint8_t const *data, uint8_t *posOut);

/**
 * Fully decodes and decompresses obfuscated bytes.
 * Returns a pre-allocated byte array (must be freed by the caller)
 * or null if the decompression failed.
 * @param self This <code>SakashoObfuscation</code>.
 */
uint8_t *SakashoObfuscation_Decode(const SakashoObfuscation *self, uint8_t const *data, int dataLen);

/**
 * Fully compresses and obfuscates raw bytes.
 *
 * <p>Returned a pre-allocated byte array (must be freed by the caller)
 * containing the compressed and obfuscated data, or null on failure.
 * <p>NOTE: The length of the array is posOut. You must trim the output.
 * Example: const len = new Uint8Array([0]);
 * let result = obfs.encode(in, size, len); result = result.subarray(0, len);
 * @param self This <code>SakashoObfuscation</code>.
 * @param posOut Array where the 0th element is the output size.
 */
uint8_t *SakashoObfuscation_Encode(const SakashoObfuscation *self, uint8_t const *data, int dataLen, int *posOut);

/**
 * Applies the conditional XOR/bit rotation operation to decode the data.
 * Reference: libsaksho.so:FUN_0004ec70, Java_jp_dena_sakasho_core_delegate_CookedResponseDelegate_cookResponse
 */
void SakashoObfuscation_XorDecodeBuffer(uint8_t *data, int dataLen, uint8_t const *table, int tableLen);

/**
 * Applies the conditional XOR/bit rotation operation to encode the data.
 * Reference: libsaksho.so:FUN_0004ebc0, Java_jp_dena_sakasho_core_http_CookedRequestBody_cookRequest
 */
void SakashoObfuscation_XorEncodeBuffer(uint8_t *data, int dataLen, uint8_t const *table, int tableLen);

#ifdef __cplusplus
}
#endif


// Generated automatically with "fut". Do not edit.
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

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
			for (int i = 0; i < encCount; i++) {
				dst[dstPos++] = dst[encPos++];
			}
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
	if (srcSize < 0)
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

void SakashoObfuscation_Initialize(SakashoObfuscation *self, const char *commonKey, const char *sessionId)
{
	self->xorLen = 0;
	for (int i = 0; i < (ptrdiff_t) strlen(commonKey) && self->xorLen < 256; i++) {
		self->xorTable[self->xorLen++] = (uint8_t) (-98 - commonKey[i]);
	}
	for (int i = 0; i < (ptrdiff_t) strlen(sessionId) && self->xorLen < 256; i++) {
		self->xorTable[self->xorLen++] = (uint8_t) sessionId[i];
	}
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





#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define DEFAULT_COMMON_KEY "9ec1c78fa2cb34e2bed5691c08432f04"

/* ---------- Utility ---------- */

static uint8_t *read_all(FILE *f, size_t *outSize) {
    uint8_t *buf = NULL;
    size_t size = 0;
    size_t cap = 0;

    for (;;) {
        if (size + 4096 > cap) {
            cap = cap ? cap * 2 : 8192;
            buf = (uint8_t *)realloc(buf, cap);
            if (!buf) return NULL;
        }
        size_t n = fread(buf + size, 1, cap - size, f);
        size += n;
        if (n == 0) break;
    }

    *outSize = size;
    return buf;
}

static int write_all(FILE *f, const uint8_t *data, size_t size) {
    return fwrite(data, 1, size, f) == size ? 0 : -1;
}

static void usage(const char *prog) {
    fprintf(stderr,
        "Usage: %s (-e|-d) [options]\n"
        "\n"
        "Options:\n"
        "  -e, --encode           Encode input\n"
        "  -d, --decode           Decode input\n"
        "  -k, --common-key KEY   Common key (default Miitomo key)\n"
        "  -s, --session-id ID    Session ID (optional)\n"
        "  -i, --input FILE       Input file (default: stdin)\n"
        "  -o, --output FILE      Output file (default: stdout)\n",
        prog);
}

/* ---------- Main ---------- */

int main(int argc, char **argv) {
    bool doEncode = false;
    bool doDecode = false;
    const char *commonKey = DEFAULT_COMMON_KEY;
    const char *sessionId = "";
    const char *inputPath = NULL;
    const char *outputPath = NULL;

    /* --- Parse args (simple, explicit) --- */
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];

        if (!strcmp(a, "-e") || !strcmp(a, "--encode")) {
            doEncode = true;
        } else if (!strcmp(a, "-d") || !strcmp(a, "--decode")) {
            doDecode = true;
        } else if (!strcmp(a, "-k") || !strcmp(a, "--common-key")) {
            if (++i >= argc) goto bad;
            commonKey = argv[i];
        } else if (!strcmp(a, "-s") || !strcmp(a, "--session-id")) {
            if (++i >= argc) goto bad;
            sessionId = argv[i];
        } else if (!strcmp(a, "-i") || !strcmp(a, "--input")) {
            if (++i >= argc) goto bad;
            inputPath = argv[i];
        } else if (!strcmp(a, "-o") || !strcmp(a, "--output")) {
            if (++i >= argc) goto bad;
            outputPath = argv[i];
        } else {
            goto bad;
        }
    }

    if (doEncode == doDecode) {
        fprintf(stderr, "Error: must specify exactly one of -e or -d\n");
        goto bad;
    }

    /* --- Open IO --- */
    FILE *in = inputPath ? fopen(inputPath, "rb") : stdin;
    if (!in) {
        perror("fopen input");
        return 1;
    }

    FILE *out = outputPath ? fopen(outputPath, "wb") : stdout;
    if (!out) {
        perror("fopen output");
        if (in != stdin) fclose(in);
        return 1;
    }

    /* --- Read input --- */
    size_t inputSize = 0;
    uint8_t *input = read_all(in, &inputSize);
    if (!input) {
        fprintf(stderr, "Failed to read input\n");
        return 1;
    }

    /* --- Init obfuscator --- */
    SakashoObfuscation obfs;
    SakashoObfuscation_Initialize(&obfs, commonKey, sessionId);

    uint8_t *result = NULL;
    size_t resultSize = 0;

    if (doDecode) {
        result = SakashoObfuscation_Decode(&obfs, input, (int)inputSize);
        if (!result) {
            fprintf(stderr, "Decode failed\n");
            return 1;
        }

        /* Size is known from varint */
        uint8_t pos = 0;
        resultSize = SakashoObfuscation_GetDecompressedSize(&obfs, input, &pos);
        if (resultSize == 0) {
            fprintf(stderr, "Invalid decompressed size\n");
            free(result);
            return 1;
        }
    } else {
        int outLen = 0;
        result = SakashoObfuscation_Encode(&obfs, input, (int)inputSize, &outLen);
        if (!result || outLen <= 0) {
            fprintf(stderr, "Encode failed\n");
            return 1;
        }
        resultSize = (size_t)outLen;
    }

    /* --- Write output --- */
    if (write_all(out, result, resultSize) != 0) {
        fprintf(stderr, "Write failed\n");
        return 1;
    }

    /* --- Cleanup --- */
    free(input);
    FuShared_Release(result);
    if (in != stdin) fclose(in);
    if (out != stdout) fclose(out);

    return 0;

bad:
    usage(argv[0]);
    return 1;
}
