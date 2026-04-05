// Generated automatically with "fut". Do not edit.
#pragma once
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
 * Length of the common key as a string.
 */
#define SakashoObfuscation_COMMON_KEY_LENGTH 32

/**
 * The common key as used in the XOR table from Miitomo.
 */
static const uint8_t SakashoObfuscation_COMMON_KEY_MIITOMO[32] = { 101, 57, 59, 109, 59, 103, 102, 56, 61, 108, 59, 60, 107, 106, 57, 108,
	60, 57, 58, 105, 104, 101, 109, 59, 110, 102, 106, 107, 108, 56, 110, 106 };

/**
 * Initializes the obfuscation class for use with Miitomo.
 * @param self This <code>SakashoObfuscation</code>.
 * @param playerSessionId The value of the player_session_id cookie,
 * or an empty string if the cookie was not set.
 */
void SakashoObfuscation_InitializeMiitomo(SakashoObfuscation *self, const char *playerSessionId);

/**
 * Builds or rebuilds the internal XOR table.
 * @param self This <code>SakashoObfuscation</code>.
 */
void SakashoObfuscation_InitializeWithKey(SakashoObfuscation *self, uint8_t const *key, uint8_t const *sessionId, int sessionIdLength);

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
 * Example: const len = new Uint32Array([0]);
 * let result = obfs.encode(in, size, len); result = result.subarray(0, len);
 * @param self This <code>SakashoObfuscation</code>.
 * @param posOut Array where the 0th element is the output size.
 */
uint8_t *SakashoObfuscation_Encode(const SakashoObfuscation *self, uint8_t const *data, int dataLen, int *posOut);

/**
 * Transform the common key from its hexadecimal string
 * form into the raw bytes used in the XOR table.
 * @param key For Miitomo, the value of this is the hexadecimal
 * string (NOT decoded from hex): 9ec1c78fa2cb34e2bed5691c08432f04
 */
void SakashoObfuscation_XformCommonKey(uint8_t *key);

/**
 * Applies the conditional XOR/bit rotation operation to decode the data.
 * Reference: libsakasho.so:FUN_0004ec70, Java_jp_dena_sakasho_core_delegate_CookedResponseDelegate_cookResponse
 */
void SakashoObfuscation_XorDecodeBuffer(uint8_t *data, int dataLen, uint8_t const *table, int tableLen);

/**
 * Applies the conditional XOR/bit rotation operation to encode the data.
 * Reference: libsakasho.so:FUN_0004ebc0, Java_jp_dena_sakasho_core_http_CookedRequestBody_cookRequest
 */
void SakashoObfuscation_XorEncodeBuffer(uint8_t *data, int dataLen, uint8_t const *table, int tableLen);

#ifdef __cplusplus
}
#endif
