package main

import (
	"bytes"
	"crypto/tls"
	"flag"
	"io"
	"log"
	"net/http"
	"net/http/httputil"
	"net/http/httptest"
	"net/url"
	"strings"
	//"strconv"

    // for access logs
	"fmt"
	"net"
	"os"
	"time"

	//"github.com/pierrec/lz4"
	"unsafe"
)

const commonKey = "9ec1c78fa2cb34e2bed5691c08432f04"

// Generated automatically with "fut". Do not edit.

const Varint_MAX_SIZE_INT = 5

type SakashoObfuscation struct {
	xorTable [256]uint8
	xorLen   int
}

// MemCpy copies count characters from the object pointed to by src to the object pointed to by dest. Both objects are
// interpreted as arrays of byte.
//
// The behavior is undefined if access occurs beyond the end of the dest array. If the objects overlap (which is a
// violation of the restrict contract), the behavior is undefined. The behavior is undefined if either dest or src is a
// null pointer.
func MemCpy(dst, src unsafe.Pointer, sz int) unsafe.Pointer {
	if dst == nil {
		panic("nil destination")
	}
	if sz == 0 || src == nil {
		return dst
	}
	bdst := unsafe.Slice((*byte)(dst), sz)
	bsrc := unsafe.Slice((*byte)(src), sz)
	copy(bdst, bsrc)
	return dst
}

func findnull[T interface{ byte | uint16 | uint32 }](str *T) int {
	if str == nil {
		return 0
	}
	var zero T
	size := unsafe.Sizeof(zero)
	i := 0
	for *str != 0 {
		str = (*T)(unsafe.Add(unsafe.Pointer(str), size))
		i++
	}
	return i
}

// StrLen returns the length of the given null-terminated byte string, that is, the number of characters in a character
// array whose first element is pointed to by str up to and not including the first null character.
//
// The behavior is undefined if str is not a pointer to a null-terminated byte string.
func StrLen(str *byte) int {
	return findnull(str)
}

func Varint_Read(data *uint8, posOut *uint8) int {
	var (
		value int = 0
		shift int = 0
		pos   int = int(*posOut)
	)
	for i := int(0); i < 5; i++ {
		var b int = int(*(*uint8)(unsafe.Add(unsafe.Pointer(data), func() int {
			p_ := &pos
			x := *p_
			*p_++
			return x
		}())))
		value |= (b & 127) << shift
		if (b & 128) == 0 {
			*posOut = uint8(int8(pos))
			return value
		}
		shift += 7
		if shift >= 35 {
			return 0
		}
	}
	return 0
}
func Varint_Write(dst *uint8, value int, offset int) int {
	var pos int = 0
	for i := int(0); i < 5; i++ {
		var b int = value & 127
		value >>= 7
		if value != 0 {
			*(*uint8)(unsafe.Add(unsafe.Pointer(dst), offset+func() int {
				p_ := &pos
				x := *p_
				*p_++
				return x
			}())) = uint8(int8(b | 128))
		} else {
			*(*uint8)(unsafe.Add(unsafe.Pointer(dst), offset+func() int {
				p_ := &pos
				x := *p_
				*p_++
				return x
			}())) = uint8(int8(b))
			return pos
		}
	}
	return 0
}
func Lz4_Decompress(src *uint8, dst *uint8, compressedSize int, dstCapacity int, srcOffset int) int {
	var (
		srcPos int = 0
		dstPos int = 0
	)
	for srcPos < compressedSize && dstPos < dstCapacity {
		if srcPos >= compressedSize {
			return -1
		}
		var token int = int(*(*uint8)(unsafe.Add(unsafe.Pointer(src), srcOffset+func() int {
			p_ := &srcPos
			x := *p_
			*p_++
			return x
		}())))
		var encCount int = token & 15
		var litCount int = token >> 4 & 15
		if litCount == 15 {
			var sum int = 0
			for {
				if srcPos >= compressedSize {
					return -1
				}
				sum = int(*(*uint8)(unsafe.Add(unsafe.Pointer(src), srcOffset+func() int {
					p_ := &srcPos
					x := *p_
					*p_++
					return x
				}())))
				litCount += sum
				if sum != 255 {
					break
				}
			}
		}
		if srcPos+litCount > compressedSize {
			return -1
		}
		if dstPos+litCount > dstCapacity {
			return -1
		}
		MemCpy(unsafe.Add(unsafe.Pointer(dst), dstPos), unsafe.Add(unsafe.Pointer(src), srcOffset+srcPos), litCount)
		srcPos += litCount
		dstPos += litCount
		if srcPos >= compressedSize {
			break
		}
		if srcPos+1 >= compressedSize {
			return -1
		}
		var back int = int(*(*uint8)(unsafe.Add(unsafe.Pointer(src), srcOffset+srcPos))) | int(*(*uint8)(unsafe.Add(unsafe.Pointer(src), srcOffset+srcPos+1)))<<8
		srcPos += 2
		if back <= 0 || back > dstPos {
			return -1
		}
		if encCount == 15 {
			var sum int = 0
			for {
				if srcPos >= compressedSize {
					return -1
				}
				sum = int(*(*uint8)(unsafe.Add(unsafe.Pointer(src), srcOffset+func() int {
					p_ := &srcPos
					x := *p_
					*p_++
					return x
				}())))
				encCount += sum
				if sum != 255 {
					break
				}
			}
		}
		encCount += 4
		if dstPos+encCount > dstCapacity {
			return -1
		}
		var encPos int = dstPos - back
		if encCount <= back {
			MemCpy(unsafe.Add(unsafe.Pointer(dst), dstPos), unsafe.Add(unsafe.Pointer(dst), encPos), encCount)
			dstPos += encCount
		} else {
			for i := int(0); i < encCount; i++ {
				*(*uint8)(unsafe.Add(unsafe.Pointer(dst), func() int {
					p_ := &dstPos
					x := *p_
					*p_++
					return x
				}())) = *(*uint8)(unsafe.Add(unsafe.Pointer(dst), func() int {
					p_ := &encPos
					x := *p_
					*p_++
					return x
				}()))
			}
		}
	}
	return dstPos
}
func Lz4_GetMaxCompressedSize(inputSize int) int {
	if inputSize > 2113929216 {
		return 0
	}
	return inputSize + inputSize/255 + 16
}
func Lz4_Compress(src *uint8, dst *uint8, srcSize int, dstCapacity int, dstOffset int) int {
	if srcSize < 0 {
		return -1
	}
	var dstPos int = 0
	var litLen int = srcSize
	var tokenLit int
	if litLen < 15 {
		tokenLit = litLen
	} else {
		tokenLit = 15
	}
	if dstPos >= dstCapacity {
		return -1
	}
	*(*uint8)(unsafe.Add(unsafe.Pointer(dst), dstOffset+func() int {
		p_ := &dstPos
		x := *p_
		*p_++
		return x
	}())) = uint8(int8(tokenLit << 4))
	if litLen >= 15 {
		var len_ int = litLen - 15
		for len_ >= 255 {
			if dstPos >= dstCapacity {
				return -1
			}
			*(*uint8)(unsafe.Add(unsafe.Pointer(dst), dstOffset+func() int {
				p_ := &dstPos
				x := *p_
				*p_++
				return x
			}())) = 255
			len_ -= 255
		}
		if dstPos >= dstCapacity {
			return -1
		}
		*(*uint8)(unsafe.Add(unsafe.Pointer(dst), dstOffset+func() int {
			p_ := &dstPos
			x := *p_
			*p_++
			return x
		}())) = uint8(int8(len_))
	}
	if dstPos+srcSize > dstCapacity {
		return -1
	}
	MemCpy(unsafe.Add(unsafe.Pointer(dst), dstOffset+dstPos), unsafe.Pointer(src), srcSize)
	dstPos += srcSize
	return dstPos
}
func SakashoObfuscation_Initialize(self *SakashoObfuscation, commonKey *byte, sessionId *byte) {
	self.xorLen = 0
	for i := int(0); i < int(int64(StrLen(commonKey))) && self.xorLen < 256; i++ {
		var c int = int(*(*byte)(unsafe.Add(unsafe.Pointer(commonKey), i)))
		self.xorTable[func() int {
			p_ := &self.xorLen
			x := *p_
			*p_++
			return x
		}()] = uint8(int8(-98 - c))
	}
	for i := int(0); i < int(int64(StrLen(sessionId))) && self.xorLen < 256; i++ {
		self.xorTable[func() int {
			p_ := &self.xorLen
			x := *p_
			*p_++
			return x
		}()] = uint8(*(*byte)(unsafe.Add(unsafe.Pointer(sessionId), i)))
	}
}
func SakashoObfuscation_XorDecode(self *SakashoObfuscation, data *uint8, dataLen int) {
	SakashoObfuscation_XorDecodeBuffer(data, dataLen, &self.xorTable[0], self.xorLen)
}
func SakashoObfuscation_XorEncode(self *SakashoObfuscation, data *uint8, dataLen int) {
	SakashoObfuscation_XorEncodeBuffer(data, dataLen, &self.xorTable[0], self.xorLen)
}
func SakashoObfuscation_GetDecompressedSize(self *SakashoObfuscation, data *uint8, posOut *uint8) int {
	var tmp [5]uint8
	MemCpy(unsafe.Pointer(&tmp[0]), unsafe.Pointer(data), 5)
	SakashoObfuscation_XorDecode(self, &tmp[0], 5)
	var posOutLocal [1]uint8
	posOutLocal[0] = 0
	if posOut == nil {
		return Varint_Read(&tmp[0], &posOutLocal[0])
	}
	return Varint_Read(&tmp[0], posOut)
}
func SakashoObfuscation_XorDecodeBuffer(data *uint8, dataLen int, table *uint8, tableLen int) {
	for i := int(0); i < dataLen; i++ {
		var (
			keyByte   int = int(*(*uint8)(unsafe.Add(unsafe.Pointer(table), (i+1)%tableLen)))
			inputByte int = int(*(*uint8)(unsafe.Add(unsafe.Pointer(data), i)))
		)
		if (keyByte & 7) == 0 {
			*(*uint8)(unsafe.Add(unsafe.Pointer(data), i)) = uint8(int8(inputByte ^ keyByte))
		} else {
			var shift int = keyByte & 7
			*(*uint8)(unsafe.Add(unsafe.Pointer(data), i)) = uint8(int8(inputByte>>(8-shift) | inputByte<<shift))
		}
	}
}
func SakashoObfuscation_XorEncodeBuffer(data *uint8, dataLen int, table *uint8, tableLen int) {
	for i := int(0); i < dataLen; i++ {
		var (
			keyByte   int = int(*(*uint8)(unsafe.Add(unsafe.Pointer(table), (i+1)%tableLen)))
			inputByte int = int(*(*uint8)(unsafe.Add(unsafe.Pointer(data), i)))
		)
		if (keyByte & 7) == 0 {
			*(*uint8)(unsafe.Add(unsafe.Pointer(data), i)) = uint8(int8(inputByte ^ keyByte))
		} else {
			var shift int = keyByte & 7
			*(*uint8)(unsafe.Add(unsafe.Pointer(data), i)) = uint8(int8(inputByte<<(8-shift) | inputByte>>shift))
		}
	}
}

//
//
//
//
//


// SakashoObfuscation wrapper with convenience methods.
// The struct is already defined in the C-transpiled code.
// We just add these methods.

// Decode fully decodes and decompresses obfuscated bytes.
// Returns the decompressed data or an error.
func (obfs *SakashoObfuscation) Decode(data []byte) ([]byte, error) {
	if len(data) == 0 {
		return nil, fmt.Errorf("empty data")
	}

	// Copy input (XOR decode modifies in-place)
	decodedData := make([]byte, len(data))
	copy(decodedData, data)

	// XOR decode in-place
	SakashoObfuscation_XorDecode(
		obfs,
		(*uint8)(unsafe.Pointer(&decodedData[0])),
		len(decodedData),
	)

	// Read varint to get decompressed size
	posOut := uint8(0)
	decompressedSize := Varint_Read(
		(*uint8)(unsafe.Pointer(&decodedData[0])),
		&posOut,
	)
	if decompressedSize < 1 {
		return nil, fmt.Errorf("invalid varint")
	}

	// Extract compressed data (skip varint)
	compressedData := decodedData[posOut:]

	// Decompress
	output := make([]byte, decompressedSize)
	result := Lz4_Decompress(
		(*uint8)(unsafe.Pointer(&compressedData[0])),
		(*uint8)(unsafe.Pointer(&output[0])),
		len(compressedData),
		decompressedSize,
		0, // srcOffset
	)
	if result < 0 {
		return nil, fmt.Errorf("lz4 decompress failed")
	}

	return output[:result], nil
}

// Encode fully compresses and obfuscates raw bytes.
// Returns the encoded data or an error.
func (obfs *SakashoObfuscation) Encode(data []byte) ([]byte, error) {
	if len(data) < 0 {
		return nil, fmt.Errorf("invalid data size")
	}

	// Get max compressed size bound
	maxCompressed := Lz4_GetMaxCompressedSize(len(data))
	if maxCompressed <= 0 {
		return nil, fmt.Errorf("lz4 bound failed")
	}

	// Allocate buffer for varint + compressed
	// We'll use a temporary buffer larger than needed, then trim
	tempBuf := make([]byte, 5+maxCompressed) // 5 = max varint size

	// Write varint (decompressed size)
	varintSize := Varint_Write(
		(*uint8)(unsafe.Pointer(&tempBuf[0])),
		len(data),
		0, // offset
	)
	if varintSize <= 0 {
		return nil, fmt.Errorf("varint write failed")
	}

	// Compress into buffer after varint
	compressedSize := Lz4_Compress(
		(*uint8)(unsafe.Pointer(&data[0])),
		(*uint8)(unsafe.Pointer(&tempBuf[0])),
		len(data),
		maxCompressed,
		varintSize, // dstOffset
	)
	if compressedSize <= 0 {
		return nil, fmt.Errorf("lz4 compress failed")
	}

	totalSize := varintSize + compressedSize

	// Trim to exact size
	encodedData := tempBuf[:totalSize]

	// XOR encode in-place
	SakashoObfuscation_XorEncode(
		obfs,
		(*uint8)(unsafe.Pointer(&encodedData[0])),
		len(encodedData),
	)

	// Make a copy to return (safe ownership)
	result := make([]byte, len(encodedData))
	copy(result, encodedData)

	return result, nil
}

// Initialize builds the XOR table from common key and session ID.
// Wrapper for convenience (C version takes *byte).
func (obfs *SakashoObfuscation) Initialize(commonKey, sessionID string) {
	// Create C-compatible null-terminated strings
	ckBytes := append([]byte(commonKey), 0)
	sidBytes := append([]byte(sessionID), 0)

	SakashoObfuscation_Initialize(
		obfs,
		(*byte)(unsafe.Pointer(&ckBytes[0])),
		(*byte)(unsafe.Pointer(&sidBytes[0])),
	)
}


// Function to detect and handle interruptions in the data
func detectInterruptions(data []byte, cutoffs []string) ([]byte, bool) {
    for _, cutoff := range cutoffs {
        index := bytes.Index(data, []byte(cutoff))
        if index != -1 {
            return data[:index], true
        }
    }
    return data, false
}

var cutoffs []string = []string{
	"<!doctype",
}

// proxyHandler handles incoming requests, decodes them, forwards to upstream, and re-encodes responses.
func proxyHandler(upstreamURL *url.URL, certFile, keyFile string) func(w http.ResponseWriter, r *http.Request) {
	upstreamProxy := httputil.NewSingleHostReverseProxy(upstreamURL)
	upstreamProxy.Transport = &http.Transport{
		TLSClientConfig: &tls.Config{InsecureSkipVerify: true},
		Proxy: http.ProxyFromEnvironment, // Allow proxy config from env
	}

	// Override the Director function to ensure we pass the original URL's host header
    upstreamProxy.Director = func(req *http.Request) {
        // Set the scheme and host to the upstream server's
        req.URL.Scheme = upstreamURL.Scheme
        req.URL.Host = upstreamURL.Host
        req.Host = upstreamURL.Host

        // Copy over the original path and query
        req.URL.Path = req.URL.Path
        req.URL.RawQuery = req.URL.RawQuery

        // Set the Host header explicitly
        req.Header.Set("Host", upstreamURL.Host)

        //req.Header.Del("Content-Length")
    }

    return func(w http.ResponseWriter, r *http.Request) {
        sessionIDCookie, _ := r.Cookie("player_session_id")
        userAgent := r.Header.Get("User-Agent")

        // Bypass de/obfuscation logic if sessionID is missing, User-Agent contains "SakashoClient", or it's /v1/session
        if sessionIDCookie == nil || !strings.Contains(userAgent, "SakashoClient") || r.URL.Path == "/v1/session" {
            // Directly proxy without de/obfuscation
            upstreamProxy.ServeHTTP(w, r)
            return
        }

        sessionID := sessionIDCookie.Value

        // Build obfuscator
		obfs := &SakashoObfuscation{}
		obfs.Initialize(commonKey, sessionID)
        //xorTable := buildXorTable(commonKey, sessionID)

        // Properly handle empty request bodies
        if r.Body != nil {
            buf := new(bytes.Buffer)
            _, err := io.Copy(buf, r.Body)
            if err != nil && err != io.EOF {
                http.Error(w, "Failed to read request body", http.StatusInternalServerError)
                return
            }

            // Only decode if the body is not empty
            if buf.Len() > 0 {
                //decodedBody, err := decodeAndDecompress(buf, xorTable)
                decodedBody, err := obfs.Decode(buf.Bytes())
                if err != nil {
                    http.Error(w, "Failed to decode request", http.StatusInternalServerError)
                    return
                }


                //r.Header.Set("Content-Length", strconv.Itoa(len(decodedBody)))

                // Replace body with decoded data
                r.Body = io.NopCloser(bytes.NewReader(decodedBody))

                // Remove Content-Length header since the body size has changed
                r.Header.Del("Content-Length")
                r.ContentLength = -1
            }
        }

        // Modify headers for upstream server if necessary
        r.Host = upstreamURL.Host
        r.URL.Scheme = upstreamURL.Scheme
        r.URL.Host = upstreamURL.Host

        // Set the Host header to match the upstream server
        r.Header.Set("Host", upstreamURL.Host)

        // Capture the upstream response
        rec := httptest.NewRecorder()
        upstreamProxy.ServeHTTP(rec, r)

        // Pass through upstream headers to client
        for k, v := range rec.Header() {
            w.Header()[k] = v
        }

        // Re-encode the response body
        if rec.Body != nil && rec.Body.Len() > 0 {
            //encodedBody, err := compressAndEncode(rec.Body.Bytes(), xorTable)
 			encodedBody, err := obfs.Encode(rec.Body.Bytes())
            if err != nil {
                http.Error(w, "Failed to encode response", http.StatusInternalServerError)
                return
            }

            // Copy encoded response to original response writer
            w.WriteHeader(rec.Code)
            /*for k, v := range rec.Header() {
                w.Header()[k] = v
            }*/
            w.Write(encodedBody)
        }
    }

}

func main() {
	// Argument parsing for certificate, upstream, and hostname to client
	var upstreamAddr string
	var hostname string
	var certFile string
	var keyFile string

	flag.StringVar(&upstreamAddr, "upstream", "https://upstream.server", "Upstream server URL")
	flag.StringVar(&hostname, "hostname", "localhost:8080", "Hostname and port for the client to connect to")
	flag.StringVar(&certFile, "cert", "cert.pem", "TLS certificate file")
	flag.StringVar(&keyFile, "key", "key.pem", "TLS key file")
	flag.Parse()

	upstreamURL, err := url.Parse(upstreamAddr)
	if err != nil {
		log.Fatalf("Invalid upstream URL: %v", err)
	}

	http.Handle("/", logRequest(http.HandlerFunc(proxyHandler(upstreamURL, certFile, keyFile))))

	log.Printf("Starting proxy server on %s", hostname)
	log.Fatal(http.ListenAndServeTLS(hostname, certFile, keyFile, nil))
}



// fancy access logs
const (
	// ANSI color codes for access logs
	ANSIReset     = "\033[0m"
	ANSIRed       = "\033[31m"
	ANSIGreen     = "\033[32m"
	ANSIYellow    = "\033[33m"
	ANSIPurple    = "\033[35m"
	ANSIFaint     = "\033[2m"
	ANSIBold      = "\033[1m"
	ANSICyan      = "\033[36m"
	ANSIBgRed     = "\033[101m"
	ANSIBgBlue    = "\033[104m"
	ANSIBgMagenta = "\033[105m"
)

func isColorTerminal() bool {
	// NOTE: hack
	return os.Getenv("TERM") == "xterm-256color"
}

// getClientIP retrieves the client IP address from the request,
// considering the X-Forwarded-For header if present.
func getClientIP(r *http.Request) string {
	host, _, _ := net.SplitHostPort(r.RemoteAddr)
	return host
}

// responseWriter is a custom http.ResponseWriter that captures the status code
type responseWriter struct {
	http.ResponseWriter
	statusCode int
}

// newResponseWriter creates a new responseWriter
func newResponseWriter(w http.ResponseWriter) *responseWriter {
	return &responseWriter{w, http.StatusOK}
}

// WriteHeader captures the status code
func (rw *responseWriter) WriteHeader(code int) {
	rw.statusCode = code
	rw.ResponseWriter.WriteHeader(code)
}

// logRequest logs each request in Apache/Nginx standard format with ANSI colors
func logRequest(handler http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		start := time.Now()
		rw := newResponseWriter(w)
		handler.ServeHTTP(rw, r)
		status := rw.statusCode

		latency := time.Since(start)
		clientIP := getClientIP(r)

		if isColorTerminal() {
			statusColor := ANSIGreen

			// Determine the status color
			if status >= 400 && status < 500 {
				statusColor = ANSIYellow
			} else if status >= 500 {
				statusColor = ANSIRed
			}
			latencyColor := getLatencyGradientColor(latency)

			clientIPColor := ANSICyan
			if r.Header.Get("X-Forwarded-For") != "" {
				clientIPColor = ANSIBgMagenta
			}

			var query string
			if r.URL.RawQuery != "" {
				query += "?"
			}
			query += r.URL.RawQuery
			queryColored := colorQueryParameters(query)

			// so many colors.....
			fmt.Println(clientIPColor + clientIP + ANSIReset +
				" - - [" + start.Format("02/Jan/2006:15:04:05 -0700") + "] \"" +
				ANSIGreen + r.Method + " " + r.URL.Path + queryColored + " " + ANSIReset +
				ANSIFaint + r.Proto + ANSIReset + "\" " +
				statusColor + fmt.Sprint(status) + ANSIReset + " " +
				fmt.Sprint(r.ContentLength) + " \"" +
				ANSIPurple + r.Referer() + ANSIReset + "\" \"" +
				ANSIFaint + r.UserAgent() + ANSIReset + "\" " +
				latencyColor + fmt.Sprint(latency) + ANSIReset)
		} else {
			// apache/nginx request format with latency at the end
			fmt.Println(clientIP + " - - [" + start.Format("02/Jan/2006:15:04:05 -0700") + "] \"" +
				r.Method + " " + r.RequestURI + " " + r.Proto + "\" " +
				fmt.Sprint(status) + " " + fmt.Sprint(r.ContentLength) + " \"" +
				r.Referer() + "\" \"" + r.UserAgent() + "\" " +
				fmt.Sprint(latency))
		}
	})
}

// Color ranges for latency gradient
var latencyColors = []string{
	"\033[38;5;39m",  // Blue
	"\033[38;5;51m",  // Light blue
	"\033[38;5;27m",  // Added color (Dark blue)
	"\033[38;5;82m",  // Green
	"\033[38;5;34m",  // Added color (Forest green)
	"\033[38;5;154m", // Light green
	"\033[38;5;220m", // Yellow
	"\033[38;5;208m", // Orange
	"\033[38;5;198m", // Light red
}

// getLatencyGradientColor returns a gradient color based on the latency
func getLatencyGradientColor(latency time.Duration) string {
	millis := latency.Milliseconds()
	// Define latency thresholds
	thresholds := []int64{40, 60, 85, 100, 150, 230, 400, 600}

	for i, threshold := range thresholds {
		if millis < threshold {
			return latencyColors[i]
		}
	}
	return latencyColors[len(latencyColors)-1]
}

// colorQueryParameters colors the query parameters
func colorQueryParameters(query string) string {
	if query == "" {
		return ""
	}
	// NOTE: the question mark and first query key are colored the same
	params := strings.Split(query, "&")
	var coloredParams []string
	for _, param := range params {
		keyValue := strings.Split(param, "=")
		if len(keyValue) == 2 {
			coloredParams = append(coloredParams, fmt.Sprintf("%s%s%s=%s%s%s", ANSICyan, keyValue[0], ANSIReset, ANSIYellow, keyValue[1], ANSIReset))
		} else {
			coloredParams = append(coloredParams, param)
		}
	}
	return strings.Join(coloredParams, "&")
}
