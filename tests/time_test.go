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

func prepTime(t *testing.T, live bool) (string, string, string) {
	t.Helper()
	root := os.Getenv("CDH_PROJECT_DIR")
	src := os.Getenv("CDH_OVERT_DIR")
	out := os.Getenv("CDH_TIME_OUT")
	adb := os.Getenv("CDH_ADB")
	serial := os.Getenv("CDH_ADB_SERIAL")
	if root == "" || src == "" || out == "" || adb == "" || serial == "" {
		t.Fatal("explicit project, source, output, adb and serial required")
	}
	if err := os.MkdirAll(out, 0o755); err != nil {
		t.Fatal(err)
	}
	tool := filepath.Join(root, "output/toolchains/darwin-arm64/ndk29/toolchains/llvm/prebuilt/darwin-x86_64/bin/aarch64-linux-android26-clang++")
	bin := filepath.Join(out, "overt-time-check")
	args := []string{"-std=c++17", "-O0", "-static-libstdc++", "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections"}
	if live {
		bin += "-live"
		args = append(args, "-DCDH_TIME_LIVE=1")
	}
	for _, part := range []string{"zconfig", "zlog", "zlibc", "zstd", "zcore", "zinfo"} {
		args = append(args, "-I"+filepath.Join(src, part, "src/main/cpp"))
	}
	args = append(args, "-I"+filepath.Join(src, "zcore/src/main/cpp/include"),
		filepath.Join(src, "tests/native/time_reply.cpp"), filepath.Join(src, "zinfo/src/main/cpp/zTimeInfo.cpp"),
		filepath.Join(src, "zstd/src/main/cpp/zStdUtil.cpp"),
		filepath.Join(src, "zlibc/src/main/cpp/zLibc.cpp"))
	if live {
		args = append(args, filepath.Join(src, "zcore/src/main/cpp/zHttps.cpp"))
		for _, lib := range []string{"libmbedtls.a", "libmbedx509.a", "libmbedcrypto.a"} {
			args = append(args, filepath.Join(src, "zcore/src/main/cpp/lib/mbedtls", lib))
		}
	}
	args = append(args, "-o", bin)
	cmd := exec.Command(tool, args...)
	cmd.Env = append(os.Environ(), "TMPDIR="+out, "TMP="+out, "TEMP="+out, "GOTMPDIR="+out)
	if data, err := cmd.CombinedOutput(); err != nil {
		t.Fatalf("native compile: %v\n%s", err, data)
	}
	guest := "/data/local/tmp/overt-time-check"
	if live {
		guest += "-live"
	}
	if uid := strings.TrimSpace(string(timeRun(t, adb, serial, "shell", "id", "-u"))); uid != "2000" {
		t.Fatalf("shell UID=%s", uid)
	}
	if mode := strings.TrimSpace(string(timeRun(t, adb, serial, "shell", "getenforce"))); mode != "Enforcing" {
		t.Fatalf("SELinux=%s", mode)
	}
	timeRun(t, adb, serial, "shell", "test", "!", "-e", guest)
	timeRun(t, adb, serial, "push", bin, guest)
	t.Cleanup(func() { timeRun(t, adb, serial, "shell", "rm", "-f", guest) })
	timeRun(t, adb, serial, "shell", "chmod", "0755", guest)
	return guest, adb, serial
}

func timeRun(t *testing.T, adb, serial string, args ...string) []byte {
	t.Helper()
	data, err := exec.Command(adb, append([]string{"-s", serial}, args...)...).CombinedOutput()
	if err != nil {
		t.Fatalf("native run: %v\n%s", err, data)
	}
	return data
}

func TestTimeReply(t *testing.T) {
	guest, adb, serial := prepTime(t, false)
	pin := "AFFCB21975697A3E70BAB083EDBF1587806A65AF90B29B0D60206563A703CBA7"
	body := `{"server_time":1700000000123}`
	for _, c := range []struct{ name, body, pin, tls, status, err, want string }{
		{"current", body, pin, "1", "200", "", "1700000000"},
		{"old", body, "604D2DE1AD32FF364041831DE23CBFC2C48AD5DEF8E665103691B6472D07D4D0", "1", "200", "", "1700000000"},
		{"wrong", body, "00", "1", "200", "", "-1"},
		{"tls", body, pin, "0", "200", "", "-1"},
		{"http", body, pin, "1", "503", "", "-1"},
		{"read", body, pin, "1", "200", "read failed", "-1"},
		{"empty", `{}`, pin, "1", "200", "", "-1"},
		{"negative", `{"server_time":-1}`, pin, "1", "200", "", "-1"},
		{"string", `{"server_time":"1700000000123"}`, pin, "1", "200", "", "-1"},
		{"float", `{"server_time":1700000000123.5}`, pin, "1", "200", "", "-1"},
		{"zero", `{"server_time":0}`, pin, "1", "200", "", "-1"},
		{"json", `{`, pin, "1", "200", "", "-1"},
	} {
		t.Run(c.name, func(t *testing.T) {
			parts := []string{guest, c.body, c.pin, c.tls, c.status, c.err}
			for i := range parts {
				parts[i] = shellArg(parts[i])
			}
			data := timeRun(t, adb, serial, "shell", strings.Join(parts, " "))
			var got map[string]map[string]string
			if err := json.Unmarshal(data, &got); err != nil {
				t.Fatalf("native result: %v\n%s", err, data)
			}
			t.Logf("remote=%s", got["remote_current_time"]["value"])
			if val := got["remote_current_time"]["value"]; val != c.want {
				t.Errorf("remote=%s, want %s", val, c.want)
			}
		})
	}
}

func TestTimeLive(t *testing.T) {
	guest, adb, serial := prepTime(t, true)
	data := timeRun(t, adb, serial, "shell", guest)
	t.Log(string(data))
	var got struct {
		TLS    string `json:"tls"`
		Status string `json:"status"`
		Pin    string `json:"pin"`
		Error  string `json:"error"`
		Remote string `json:"remote"`
		Local  string `json:"local"`
	}
	if err := json.Unmarshal(data, &got); err != nil {
		t.Fatal(err)
	}
	if got.TLS != "true" || got.Status != "200" || got.Error != "" || got.Pin != "AFFCB21975697A3E70BAB083EDBF1587806A65AF90B29B0D60206563A703CBA7" {
		t.Fatalf("trusted network response failed: %s", data)
	}
	remote, err := strconv.ParseInt(got.Remote, 10, 64)
	if err != nil || remote <= 0 {
		t.Fatalf("remote time unavailable: %s %v", got.Remote, err)
	}
	local, err := strconv.ParseInt(got.Local, 10, 64)
	if err != nil || remote-local > 60 || local-remote > 60 {
		t.Fatalf("clock delta=%d: %v", remote-local, err)
	}
}
