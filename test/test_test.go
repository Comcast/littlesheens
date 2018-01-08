package main

import "testing"

func TestBasic(t *testing.T) {
	testBasic(t)
}

func BenchmarkBasic(b *testing.B) {
	benchmarkBasic(b)
}

func TestSandboxLeak(t *testing.T) {
	testSanboxLeak(t)
}
