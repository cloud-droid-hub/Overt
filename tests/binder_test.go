// Copyright (c) 2025-2026 feicong(https://github.com/feicong/feicong-course)
package ssltest

import (
	"encoding/json"
	"os"
	"os/exec"
	"path/filepath"
	"strconv"
	"strings"
	"testing"
)

func TestBinderReply(t *testing.T) {
	root := os.Getenv("CDH_PROJECT_DIR")
	src := os.Getenv("CDH_OVERT_DIR")
	out := os.Getenv("CDH_IPC_OUT")
	adb := os.Getenv("CDH_ADB")
	serial := os.Getenv("CDH_ADB_SERIAL")
	if root == "" || src == "" || out == "" || adb == "" || serial == "" {
		t.Fatal("explicit project, source, output, adb and serial required")
	}
	if uid := strings.TrimSpace(string(timeRun(t, adb, serial, "shell", "id", "-u"))); uid != "2000" {
		t.Fatalf("shell UID=%s", uid)
	}
	if mode := strings.TrimSpace(string(timeRun(t, adb, serial, "shell", "getenforce"))); mode != "Enforcing" {
		t.Fatalf("SELinux=%s", mode)
	}
	if err := os.MkdirAll(out, 0o755); err != nil {
		t.Fatal(err)
	}
	tool := filepath.Join(root, "output/toolchains/darwin-arm64/ndk29/toolchains/llvm/prebuilt/darwin-x86_64/bin/aarch64-linux-android26-clang++")
	bin := filepath.Join(out, "overt-ipc-check")
	args := []string{"-std=c++17", "-O0", "-static-libstdc++", "-Werror"}
	for _, part := range []string{"zconfig", "zlog", "zcore"} {
		args = append(args, "-I"+filepath.Join(src, part, "src/main/cpp"))
	}
	args = append(args, filepath.Join(src, "tests/native/ipc_reply.cpp"),
		filepath.Join(src, "zcore/src/main/cpp/zBinder.cpp"), filepath.Join(src, "zlog/src/main/cpp/zLog.cpp"),
		"-landroid", "-llog", "-o", bin)
	cmd := exec.Command(tool, args...)
	cmd.Env = append(os.Environ(), "TMPDIR="+out, "TMP="+out, "TEMP="+out, "GOTMPDIR="+out)
	if data, err := cmd.CombinedOutput(); err != nil {
		t.Fatalf("native compile: %v\n%s", err, data)
	}
	guest := "/data/local/tmp/overt-ipc-check"
	timeRun(t, adb, serial, "shell", "test", "!", "-e", guest)
	timeRun(t, adb, serial, "push", bin, guest)
	t.Cleanup(func() { timeRun(t, adb, serial, "shell", "rm", "-f", guest) })
	timeRun(t, adb, serial, "shell", "chmod", "0755", guest)
	for _, size := range []int{16, 511, 512, 4091, 4092, 16384, 262144, 1048571} {
		t.Run(strconv.Itoa(size), func(t *testing.T) {
			data, err := exec.Command(adb, "-s", serial, "shell", guest, "reply", strconv.Itoa(size)).CombinedOutput()
			t.Logf("response=%s", data)
			if err != nil {
				t.Fatalf("real shared-memory reply: %v", err)
			}
			var row struct {
				Expected int
				Got      int
				Exact    bool
			}
			if err := json.Unmarshal(data, &row); err != nil || !row.Exact || row.Got != row.Expected || row.Got != size {
				t.Fatalf("reply must be complete: %+v %v", row, err)
			}
		})
	}
	for _, mode := range []string{"request", "oversize", "small", "short-map", "timeout"} {
		t.Run(mode, func(t *testing.T) {
			data, err := exec.Command(adb, "-s", serial, "shell", guest, mode, "16").CombinedOutput()
			t.Logf("boundary=%s", data)
			var row struct{ Rejected bool }
			if err != nil || json.Unmarshal(data, &row) != nil || !row.Rejected {
				t.Fatalf("invalid or absent reply must fail: %+v %v", row, err)
			}
		})
	}
}
