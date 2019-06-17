package main

// #cgo CFLAGS: -I..
// #cgo LDFLAGS: -L.. -lmachines -lduktape -lm
// #include<stdlib.h>
// #include"machines.h"
import "C"

import (
	"bytes"
	"encoding/json"
	"fmt"
	"log"
	"runtime"
	"testing"
	"time"
	"unsafe"
	// "unsafe"
)

func main() {
	testBasic(&testing.T{})
}

type buffer []byte

func newBuffer(siz int) buffer {
	return make([]byte, siz)
}

func (b buffer) zero() buffer {
	// https://codereview.appspot.com/137880043
	for i := range b {
		b[i] = 0
	}
	return b
}

func (b buffer) c() *C.char {
	return (*C.char)(unsafe.Pointer(&b[0]))
}

func (b buffer) bytes() []byte {
	return b[0:bytes.IndexByte(b, 0)]
}

func testBasic(t *testing.T) {
	C.mach_set_ctx(C.mach_make_ctx())
	if rc := C.mach_open(); rc != 0 {
		t.Fatal(rc)
	}

	siz := 256
	buf := newBuffer(siz)

	if rc := C.mach_eval(C.CString("JSON.stringify(1+2)"), buf.c(), C.int(siz)); rc != 0 {
		t.Fatal(rc)
	}

	buf = buf.bytes()

	var result interface{}
	if err := json.Unmarshal(buf, &result); err != nil {
		t.Fatal(err)
	}
	fmt.Printf("result: %T %v\n", result, result)
	switch vv := result.(type) {
	case float64:
		if vv != 3 {
			t.Fatal("wrong answer")
		}
	default:
		t.Fatal("wrong type")
	}

	C.mach_close()
}

func benchmarkBasic(b *testing.B) {
	C.mach_open()
	defer C.mach_close()

	siz := 256
	buf := newBuffer(siz)

	b.ResetTimer()

	for i := 0; i < b.N; i++ {
		rc := C.mach_eval(C.CString("JSON.stringify(1+2)"), buf.zero().c(), C.int(siz))
		if rc != 0 {
			b.Fatal(rc)
		}
	}
}

func testSanboxLeak(t *testing.T) {

	// This function uses the JavaScript sandbox to generate a
	// large string, which is returned (after stringifying).  That
	// processes is repeated in a loop.

	// ToDo: Use cgroups to limit memory.
	// ToDo: Much more ...

	var (
		rounds     = 10000
		gcInterval = rounds / 10
		exp        = 16
		src        = fmt.Sprintf(`var x = "!"; for (var i=0; i<%d; i++) { x += x; } JSON.stringify(x);`, exp)
		acc        = 0

		siz = 1
	)

	for i := 0; i < exp; i++ {
		siz += siz
	}
	siz += 3 // JSON

	C.mach_set_ctx(C.mach_make_ctx())
	if rc := C.mach_open(); rc != 0 {
		t.Fatal(rc)
	}

	buf := newBuffer(siz)

	then := time.Now()
	for i := 0; i < rounds; i++ {
		if rc := C.mach_eval(C.CString(src), buf.zero().c(), C.int(siz)); rc != 0 {
			t.Fatal(rc, i)
		}
		js := buf.bytes()
		acc += len(js)

		// C.free(unsafe.Pointer(&s))

		if i%gcInterval == 0 {
			runtime.GC()
		}
	}

	log.Printf("sandbox leak test allocated %0.2fMB, elapsed %v", float64(acc)/1024/1024, time.Now().Sub(then))

	C.mach_close()
}
