package main

import "unsafe"

func MemCpy(dst, src unsafe.Pointer, sz int) {
	bdst := unsafe.Slice((*byte)(dst), sz)
	bsrc := unsafe.Slice((*byte)(src), sz)
	copy(bdst, bsrc)
}

const Varint_MAX_SIZE_INT = 5
const SakashoObfuscation_COMMON_KEY_LENGTH = 32

type SakashoObfuscation struct {
	xorTable [256]uint8
	xorLen   int
}

var SakashoObfuscation_COMMON_KEY_MIITOMO [32]uint8 = [32]uint8{101, 57, 59, 109, 59, 103, 102, 56, 61, 108, 59, 60, 107, 106, 57, 108, 60, 57, 58, 105, 104, 101, 109, 59, 110, 102, 106, 107, 108, 56, 110, 106}

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
	if srcSize <= 0 {
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
func SakashoObfuscation_InitializeWithKey(self *SakashoObfuscation, key *uint8, sessionId *uint8, sessionIdLength int) {
	self.xorLen = 0
	SakashoObfuscation_AddToXorTable(self, key, 32)
	if sessionIdLength > 0 && sessionId == nil {
		panic("assert failed")
	}
	SakashoObfuscation_AddToXorTable(self, sessionId, sessionIdLength)
}
func SakashoObfuscation_AddToXorTable(self *SakashoObfuscation, b *uint8, length int) {
	for i := int(0); i < length && self.xorLen < 256; i++ {
		self.xorTable[func() int {
			p_ := &self.xorLen
			x := *p_
			*p_++
			return x
		}()] = *(*uint8)(unsafe.Add(unsafe.Pointer(b), i))
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
func SakashoObfuscation_XformCommonKey(key *uint8) {
	for i := int(0); i < 32; i++ {
		var c int = int(*(*uint8)(unsafe.Add(unsafe.Pointer(key), i)))
		*(*uint8)(unsafe.Add(unsafe.Pointer(key), i)) = uint8(int8(-98 - c))
	}
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
