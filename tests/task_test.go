// Copyright (c) 2025-2026 fei_cong(https://github.com/feicong/feicong-course)
package ssltest

import (
	"encoding/json"
	"os"
	"os/exec"
	"path/filepath"
	"testing"
)

func TestTask(t *testing.T) {
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
	ndk := filepath.Join(root, "output/toolchains/darwin-arm64/ndk29/toolchains/llvm/prebuilt/darwin-x86_64")
	tool := filepath.Join(ndk, "bin/aarch64-linux-android26-clang++")
	bin := filepath.Join(out, "overt-task-check")
	args := []string{"-std=c++17", "-O0", "-g", "-static-libstdc++", "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections", "-Wl,--wrap=_ZN7zThread14setExecuteTaskEP5zTask", "-Wl,--wrap=pthread_cond_signal", "-Wl,--wrap=_Znwm", "-Wl,--wrap=_ZdlPv", "-Wl,--wrap=_ZdlPvm"}
	for _, part := range []string{"zconfig", "zlog", "zlibc", "zstd", "zcore", "zinfo"} {
		args = append(args, "-I"+filepath.Join(src, part, "src/main/cpp"))
	}
	args = append(args, "-I"+filepath.Join(src, "zcore/src/main/cpp/include"), filepath.Join(src, "tests/native/task_life.cpp"))
	for _, file := range []string{"zThread.cpp", "zThreadPool.cpp", "zTask.cpp", "zFile.cpp"} {
		args = append(args, filepath.Join(src, "zcore/src/main/cpp", file))
	}
	args = append(args, filepath.Join(src, "zstd/src/main/cpp/zStdUtil.cpp"), filepath.Join(src, "zlibc/src/main/cpp/zLibcUtil.cpp"), "-o", bin)
	cmd := exec.Command(tool, args...)
	cmd.Env = append(os.Environ(), "TMPDIR="+out, "TMP="+out, "TEMP="+out, "GOTMPDIR="+out)
	if data, err := cmd.CombinedOutput(); err != nil {
		t.Fatalf("native compile: %s %v", data, err)
	}
	guest := "/data/local/tmp/overt-task-check"
	for _, args := range [][]string{{"shell", "mkdir", "-p", guest}, {"push", bin, guest + "/check"}, {"shell", "chmod", "0755", guest + "/check"}} {
		if data, err := exec.Command(adb, append([]string{"-s", serial}, args...)...).CombinedOutput(); err != nil {
			t.Fatalf("native transfer: %s %v", data, err)
		}
	}
	for _, name := range []string{"pool", "worker"} {
		t.Run(name, func(t *testing.T) {
			data, err := exec.Command(adb, "-s", serial, "shell", guest+"/check", name).CombinedOutput()
			t.Logf("native response: %s error=%v", data, err)
			if err != nil {
				t.Fatal(err)
			}
			var got struct {
				Accepted bool `json:"accepted"`
				Done     bool `json:"done"`
				Freed    bool `json:"freed"`
				Ran      int  `json:"ran"`
			}
			if err := json.Unmarshal(data, &got); err != nil {
				t.Fatal(err)
			}
			if !got.Accepted || !got.Done || !got.Freed || got.Ran != 1 {
				t.Fatalf("task did not complete exactly once: %s", data)
			}
		})
	}
}
