#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "transpiled/SakashoObfuscation.h"

// The C source is included DIRECTLY.
#include "transpiled/SakashoObfuscation.c"

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
	char xformKey[sizeof(DEFAULT_COMMON_KEY)];
	strncpy(xformKey, commonKey, sizeof(DEFAULT_COMMON_KEY));
	SakashoObfuscation_XformCommonKey(&xformKey[0]);
    SakashoObfuscation_InitializeWithKey(&obfs, &xformKey[0],
		sessionId, strnlen(sessionId, 256));

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
