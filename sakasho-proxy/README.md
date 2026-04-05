# SakashoObfuscation Reverse Proxy
This is simple reverse proxy in Go that obfuscates output data going to clients, and de-obfuscates input data coming from clients.

It allows making a revival service without the backend concerning itself with the obfuscation details.

## Transpiling to Go
Fusion does not natively support Go. It would've been possible to use the transpiled C version using Cgo, but because the code is very simple, I was able to transpile directly to Go.

The resulting code uses `unsafe`, making it.. unsafe, but it at least means Cgo is not needed.

### Steps
1. Transpile the C version:
    - The flags `NO_ALLOC`/`NO_STRING` are used to remove some dependencies.
```
fut -o SakashoObfuscation.c -D C -D NO_ALLOC -D NO_STRING -l c SakashoObfuscation.fu
```

2. Use [cxgo](https://github.com/gotranspile/cxgo) to transpile the C output.
```
cxgo file SakashoObfuscation.c
```

3. In the output `SakashoObfuscation.go`, manually inline `libc.` calls.
    - This is optional, but ensures zero dependencies are needed.
    - So pretty much just MemCpy, from `cxgo/runtime/libc/string.go`.

That output code makes a lot of use of pointers, which I'm not a fan of. Perhaps if another transpiler for Go comes out that ports from Java or TypeScript, that will produce better results.