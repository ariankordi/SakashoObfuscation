// Generated automatically with "fut". Do not edit.

/**
 * Reads and writes Base128 Varints used in Protobufs, DWARF...
 */
export class Varint {
    /**
   * @private
   */
    constructor() {
    }

    /**
     * Maximum size for a 32-bit integer as a varint.
   * @public
   * @readonly
   * @type {number}
   */
    static MAX_SIZE_INT = 5;

    /**
     * Reads a varint from data starting at posOut[0].
     * Advances posOut[0] to indicate how large the varint was.
     * Maximum is a 32-bit int. Returns 0 on invalid varint.
   * @public
   * @param {Readonly<Uint8Array>} data Byte array containing the varint.
   * @param {Uint8Array} posOut Single-element array containing the cursor.
   */
    static read(data, posOut) {
        let value = 0;
        let shift = 0;
        let pos = posOut[0];
        for (let i = 0; i < 5; i++) {
            let b = data[pos++];
            value |= (b & 127) << shift;
            if ((b & 128) == 0) {
                posOut[0] = pos;
                return value;
            }
            shift += 7;
            if (shift >= 35)
                return 0;
        }
        return 0;
    }

    /**
     * Writes a 32-bit varint to the specified output.
     * Returns varint size, or 0 on failure.
   * @public
   * @param {Uint8Array} dst
   * @param {number} value
   * @param {number} [offset=0] Offset within dst to write the varint to.
   */
    static write(dst, value, offset = 0) {
        let pos = 0;
        for (let i = 0; i < 5; i++) {
            let b = value & 127;
            value >>= 7;
            if (value != 0) {
                dst[offset + pos++] = b | 128;
            }
            else {
                dst[offset + pos++] = b;
                return pos;
            }
        }
        return 0;
    }
}

/**
 * Simple LZ4 en/decoder. Goals are to be small and safe.
 * Does not use the LZ4 frame format, and does not perform compression.
 */
export class Lz4 {
    /**
   * @private
   */
    constructor() {
    }

    /**
     * Decompresses a chunk of raw LZ4 data into the output buffer.
     * Returns the number of bytes decompressed, or -1 on corruption.
   * @public
   * @param {Readonly<Uint8Array>} src Compressed input data.
   * @param {Uint8Array} dst Decompressed output.
   * @param {number} compressedSize Length of compressed input data.
   * @param {number} dstCapacity Capacity of the output buffer.
   * @param {number} [srcOffset=0] Offset into the compressed data.
   */
    static decompress(src, dst, compressedSize, dstCapacity, srcOffset = 0) {
        let srcPos = 0;
        let dstPos = 0;
        while (srcPos < compressedSize && dstPos < dstCapacity) {
            if (srcPos >= compressedSize)
                return -1;
            let token = src[srcOffset + srcPos++];
            let encCount = token & 15;
            let litCount = token >> 4 & 15;
            if (litCount == 15) {
                let sum = 0;
                do {
                    if (srcPos >= compressedSize)
                        return -1;
                    sum = src[srcOffset + srcPos++];
                    litCount += sum;
                } while (sum == 255);
            }
            if (srcPos + litCount > compressedSize)
                return -1;
            if (dstPos + litCount > dstCapacity)
                return -1;
            dst.set(src.subarray(srcOffset + srcPos, srcOffset + srcPos + litCount), dstPos);
            srcPos += litCount;
            dstPos += litCount;
            if (srcPos >= compressedSize)
                break;
            if (srcPos + 1 >= compressedSize)
                return -1;
            let back = src[srcOffset + srcPos] | src[srcOffset + srcPos + 1] << 8;
            srcPos += 2;
            if (back <= 0 || back > dstPos)
                return -1;
            if (encCount == 15) {
                let sum = 0;
                do {
                    if (srcPos >= compressedSize)
                        return -1;
                    sum = src[srcOffset + srcPos++];
                    encCount += sum;
                } while (sum == 255);
            }
            encCount += 4;
            if (dstPos + encCount > dstCapacity)
                return -1;
            let encPos = dstPos - back;
            if (encCount <= back) {
                dst.set(dst.subarray(encPos, encPos + encCount), dstPos);
                dstPos += encCount;
            }
            else {
                for (let i = 0; i < encCount; i++)
                    dst[dstPos++] = dst[encPos++];
            }
        }
        return dstPos;
    }

    /**
     * Gets the maximum size of a compressed buffer, same as LZ4_compressBound.
   * @public
   * @param {number} inputSize
   */
    static getMaxCompressedSize(inputSize) {
        return inputSize > 2113929216 ? 0 : inputSize + (inputSize / 255 | 0) + 16;
    }

    /**
     * Encodes input into raw LZ4 block format with no framing.
     *
     * <p>Returns the number of bytes written, or -1 on failure.
     * <p>This does not actually compress the data, leaving it
     * larger than it came in just for interoperability.
   * @public
   * @param {Readonly<Uint8Array>} src Input data.
   * @param {Uint8Array} dst Destination to write compressed data to.
   * @param {number} srcSize Size of input data buffer.
   * @param {number} dstCapacity Capacity of the output buffer.
   * @param {number} [dstOffset=0] Offset into the destination buffer.
   */
    static compress(src, dst, srcSize, dstCapacity, dstOffset = 0) {
        if (srcSize <= 0)
            return -1;
        let dstPos = 0;
        let litLen = srcSize;
        let tokenLit = litLen < 15 ? litLen : 15;
        if (dstPos >= dstCapacity)
            return -1;
        dst[dstOffset + dstPos++] = tokenLit << 4;
        if (litLen >= 15) {
            let len = litLen - 15;
            while (len >= 255) {
                if (dstPos >= dstCapacity)
                    return -1;
                dst[dstOffset + dstPos++] = 255;
                len -= 255;
            }
            if (dstPos >= dstCapacity)
                return -1;
            dst[dstOffset + dstPos++] = len;
        }
        if (dstPos + srcSize > dstCapacity)
            return -1;
        dst.set(src.subarray(0, srcSize), dstOffset + dstPos);
        dstPos += srcSize;
        return dstPos;
    }
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
export class SakashoObfuscation {

    /**
     * Length of the common key as a string.
   * @public
   * @readonly
   * @type {number}
   */
    static COMMON_KEY_LENGTH = 32;

    /**
     * The common key as used in the XOR table from Miitomo.
   * @public
   * @readonly
   * @type {Readonly<Uint8Array>}
   */
    static COMMON_KEY_MIITOMO = new Uint8Array([101, 57, 59, 109, 59, 103, 102, 56, 61, 108, 59, 60, 107, 106, 57, 108,
        60, 57, 58, 105, 104, 101, 109, 59, 110, 102, 106, 107, 108, 56, 110, 106]);

    /**
     * Maximum for the XOR table length. Official player_session_id
     * cookies may push this. So if you're having issues, try increasing this.
   * @readonly
   * @type {number}
   */
    static #XOR_TABLE_LENGTH_MAX = 256;
    /**
   * @readonly
   * @type {Uint8Array}
   */
    #xorTable = new Uint8Array(256);
    /**
   * @type {number}
   */
    #xorLen;

    /**
     * Initializes the obfuscation class for use with Miitomo.
   * @public
   * @param {string} playerSessionId The value of the player_session_id cookie,
   * or an empty string if the cookie was not set.
   */
    initializeMiitomo(playerSessionId) {
        this.initializeWithKey(SakashoObfuscation.COMMON_KEY_MIITOMO, null, 0);
        for (let i = 0; i < playerSessionId.length && this.#xorLen < 256; i++)
            this.#xorTable[this.#xorLen++] = playerSessionId.charCodeAt(i);
    }

    /**
     * Builds or rebuilds the internal XOR table.
   * @public
   * @param {Readonly<Uint8Array>} key
   * @param {Readonly<Uint8Array> | null} sessionId
   * @param {number} sessionIdLength
   */
    initializeWithKey(key, sessionId, sessionIdLength) {
        this.#xorLen = 0;
        this.#addToXorTable(key, 32);
        console.assert(sessionIdLength <= 0 || sessionId != null);
        this.#addToXorTable(sessionId, sessionIdLength);
    }

    /**
     * Adds bytes to the XOR table, making sure to not overflow it.
     * Typically the transformed common key is added, and then the session ID.
   * @param {Readonly<Uint8Array>} b
   * @param {number} length
   */
    #addToXorTable(b, length) {
        for (let i = 0; i < length && this.#xorLen < 256; i++)
            this.#xorTable[this.#xorLen++] = b[i];
    }

    /**
     * Applies XOR decoding to the buffer in-place.
   * @public
   * @param {Uint8Array} data
   * @param {number} dataLen
   */
    xorDecode(data, dataLen) {
        SakashoObfuscation.xorDecodeBuffer(data, dataLen, this.#xorTable, this.#xorLen);
    }

    /**
     * Applies XOR encoding to the buffer in-place.
   * @public
   * @param {Uint8Array} data
   * @param {number} dataLen
   */
    xorEncode(data, dataLen) {
        SakashoObfuscation.xorEncodeBuffer(data, dataLen, this.#xorTable, this.#xorLen);
    }

    /**
     * Gets decompressed size from obfuscated/compressed
     * data, or 0 if the size varint is invalid.
   * @public
   * @param {Readonly<Uint8Array>} data
   * @param {Uint8Array | null} [posOut=null]
   */
    getDecompressedSize(data, posOut = null) {
        const tmp = new Uint8Array(5);
        tmp.set(data.subarray(0, 5));
        this.xorDecode(tmp, 5);
        const posOutLocal = new Uint8Array(1);
        posOutLocal[0] = 0;
        return posOut == null ? Varint.read(tmp, posOutLocal) : Varint.read(tmp, posOut);
    }

    /**
     * Fully decodes and decompresses obfuscated bytes.
     * Returns a pre-allocated byte array (must be freed by the caller)
     * or null if the decompression failed.
   * @public
   * @param {Readonly<Uint8Array>} data
   * @param {number} dataLen
   */
    decode(data, dataLen) {
        let tmp = new Uint8Array(dataLen);
        tmp.set(data.subarray(0, dataLen));
        this.xorDecode(tmp, dataLen);
        const posOut = new Uint8Array(1);
        posOut[0] = 0;
        let size = Varint.read(tmp, posOut);
        if (size <= 0)
            return null;
        console.assert(size <= 104857600);
        let output = new Uint8Array(size);
        let decompressed = Lz4.decompress(tmp, output, dataLen - posOut[0], size, posOut[0]);
        if (decompressed <= 0)
            return null;
        return output;
    }

    /**
     * Fully compresses and obfuscates raw bytes.
     *
     * <p>Returned a pre-allocated byte array (must be freed by the caller)
     * containing the compressed and obfuscated data, or null on failure.
     * <p>NOTE: The length of the array is posOut. You must trim the output.
     * Example: const len = new Uint32Array([0]);
     * let result = obfs.encode(in, size, len); result = result.subarray(0, len);
   * @public
   * @param {Readonly<Uint8Array>} data
   * @param {number} dataLen
   * @param {Int32Array} posOut Array where the 0th element is the output size.
   */
    encode(data, dataLen, posOut) {
        let maxCompressed = Lz4.getMaxCompressedSize(dataLen);
        if (maxCompressed <= 0)
            return null;
        let buffer = new Uint8Array(5 + maxCompressed);
        let varintSize = Varint.write(buffer, dataLen, 0);
        if (varintSize <= 0)
            return null;
        let compressedSize = Lz4.compress(data, buffer, dataLen, maxCompressed, varintSize);
        if (compressedSize <= 0)
            return null;
        let totalSize = varintSize + compressedSize;
        this.xorEncode(buffer, totalSize);
        posOut[0] = totalSize;
        return buffer;
    }

    /**
     * Transform the common key from its hexadecimal string
     * form into the raw bytes used in the XOR table.
   * @public
   * @param {Uint8Array} key For Miitomo, the value of this is the hexadecimal
   * string (NOT decoded from hex): 9ec1c78fa2cb34e2bed5691c08432f04
   */
    static xformCommonKey(key) {
        for (let i = 0; i < 32; i++) {
            let c = key[i];
            key[i] = (-98 - c) & 255;
        }
    }

    /**
     * Applies the conditional XOR/bit rotation operation to decode the data.
     * Reference: libsakasho.so:FUN_0004ec70, Java_jp_dena_sakasho_core_delegate_CookedResponseDelegate_cookResponse
   * @public
   * @param {Uint8Array} data
   * @param {number} dataLen
   * @param {Readonly<Uint8Array>} table
   * @param {number} tableLen
   */
    static xorDecodeBuffer(data, dataLen, table, tableLen) {
        for (let i = 0; i < dataLen; i++) {
            let keyByte = table[(i + 1) % tableLen];
            let inputByte = data[i];
            if ((keyByte & 7) == 0) {
                data[i] = inputByte ^ keyByte;
            }
            else {
                let shift = keyByte & 7;
                data[i] = (inputByte >> (8 - shift) | inputByte << shift) & 255;
            }
        }
    }

    /**
     * Applies the conditional XOR/bit rotation operation to encode the data.
     * Reference: libsakasho.so:FUN_0004ebc0, Java_jp_dena_sakasho_core_http_CookedRequestBody_cookRequest
   * @public
   * @param {Uint8Array} data
   * @param {number} dataLen
   * @param {Readonly<Uint8Array>} table
   * @param {number} tableLen
   */
    static xorEncodeBuffer(data, dataLen, table, tableLen) {
        for (let i = 0; i < dataLen; i++) {
            let keyByte = table[(i + 1) % tableLen];
            let inputByte = data[i];
            if ((keyByte & 7) == 0) {
                data[i] = inputByte ^ keyByte;
            }
            else {
                let shift = keyByte & 7;
                data[i] = (inputByte << (8 - shift) | inputByte >> shift) & 255;
            }
        }
    }
}
