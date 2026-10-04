// Copyright (c) 2025-2026 fei_cong(https://github.com/feicong/feicong-course)
package ssltest

import (
	"encoding/json"
	"os"
	"os/exec"
	"path/filepath"
	"testing"
)

func TestMapLife(t *testing.T) {
	root := os.Getenv("CDH_PROJECT_DIR")
	src := os.Getenv("CDH_OVERT_DIR")
	out := os.Getenv("CDH_SSL_OUT")
	adb := os.Getenv("CDH_ADB")
	serial := os.Getenv("CDH_ADB_SERIAL")
	if root == "" || src == "" || out == "" || adb == "" || serial == "" {
		t.Fatal("explicit project, source, output, adb and serial required")
	}
	if err := os.MkdirAll(out, 0o755); err != nil {
		t.Fatal(err)
	}
	tool := filepath.Join(root, "output/toolchains/darwin-arm64/ndk29/toolchains/llvm/prebuilt/darwin-x86_64/bin/aarch64-linux-android26-clang++")
	bin := filepath.Join(out, "overt-map-check")
	args := []string{"-std=c++17", "-O0", "-static-libstdc++", "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections"}
	for _, part := range []string{"zconfig", "zlog", "zlibc", "zstd", "zcore", "zinfo"} {
		args = append(args, "-I"+filepath.Join(src, part, "src/main/cpp"))
	}
	args = append(args, "-I"+filepath.Join(src, "zcore/src/main/cpp/include"),
		filepath.Join(src, "tests/native/map_life.cpp"),
		filepath.Join(src, "zcore/src/main/cpp/zProcMaps.cpp"),
		filepath.Join(src, "zcore/src/main/cpp/zFile.cpp"),
		filepath.Join(src, "zstd/src/main/cpp/zStdUtil.cpp"),
		filepath.Join(src, "zlibc/src/main/cpp/zLibc.cpp"), "-o", bin)
	cmd := exec.Command(tool, args...)
	cmd.Env = append(os.Environ(), "TMPDIR="+out, "TMP="+out, "TEMP="+out, "GOTMPDIR="+out)
	if data, err := cmd.CombinedOutput(); err != nil {
		t.Fatalf("native compile: %s %v", data, err)
	}
	guest := "/data/local/tmp/overt-map-check"
	for _, args := range [][]string{{"push", bin, guest}, {"shell", "chmod", "0755", guest}} {
		if data, err := exec.Command(adb, append([]string{"-s", serial}, args...)...).CombinedOutput(); err != nil {
			t.Fatalf("native transfer: %s %v", data, err)
		}
	}
	data, err := exec.Command(adb, "-s", serial, "shell", guest).CombinedOutput()
	t.Logf("native response: %s error=%v", data, err)
	if err != nil {
		t.Fatal(err)
	}
	var got map[string]bool
	if err := json.Unmarshal(data, &got); err != nil {
		t.Fatal(err)
	}
	if len(got) != 3 || !got["stable"] || !got["missing"] || !got["changed"] {
		t.Fatalf("mapping returned temporary storage: %s", data)
	}
}
