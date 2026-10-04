// Copyright (c) 2025-2026 fei_cong(https://github.com/feicong/feicong-course)
package ssltest

import (
	"encoding/json"
	"os"
	"os/exec"
	"path/filepath"
	"testing"
)

func TestHTTP(t *testing.T) {
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
	bin := filepath.Join(out, "overt-http-check")
	args := []string{"-std=c++17", "-O0", "-static-libstdc++", "-Wl,--wrap=mbedtls_ssl_read"}
	for _, part := range []string{"zconfig", "zlog", "zlibc", "zstd", "zcore", "zinfo"} {
		args = append(args, "-I"+filepath.Join(src, part, "src/main/cpp"))
	}
	args = append(args, "-I"+filepath.Join(src, "zcore/src/main/cpp/include"),
		filepath.Join(src, "tests/native/http_read.cpp"),
		filepath.Join(src, "zcore/src/main/cpp/zHttps.cpp"),
		filepath.Join(src, "zstd/src/main/cpp/zStdUtil.cpp"),
		filepath.Join(src, "zlibc/src/main/cpp/zLibc.cpp"))
	for _, lib := range []string{"libmbedtls.a", "libmbedx509.a", "libmbedcrypto.a"} {
		args = append(args, filepath.Join(src, "zcore/src/main/cpp/lib/mbedtls", lib))
	}
	args = append(args, "-o", bin)
	cmd := exec.Command(tool, args...)
	cmd.Env = append(os.Environ(), "TMPDIR="+out, "TMP="+out, "TEMP="+out, "GOTMPDIR="+out)
	if data, err := cmd.CombinedOutput(); err != nil {
		t.Fatalf("native compile: %s %v", data, err)
	}
	guest := "/data/local/tmp/overt-http-check"
	for _, args := range [][]string{{"push", bin, guest}, {"shell", "chmod", "0755", guest}} {
		if data, err := exec.Command(adb, append([]string{"-s", serial}, args...)...).CombinedOutput(); err != nil {
			t.Fatalf("native transfer: %s %v", data, err)
		}
	}
	for _, c := range []struct{ label, name string }{
		{"split", "split-close"}, {"records", "many-records"},
		{"length", "length-last"}, {"eof", "until-eof"},
		{"chunked", "chunked"}, {"trunc", "truncated"},
		{"read", "read-error"}, {"timeout", "timeout"},
		{"big", "oversize"}, {"binary", "binary"},
		{"head", "head"}, {"hchunk", "head-chunked"}, {"hsplit", "head-split"},
	} {
		t.Run(c.label, func(t *testing.T) {
			name := c.name
			data, err := exec.Command(adb, "-s", serial, "shell", guest, name).CombinedOutput()
			t.Logf("native response: %s error=%v", data, err)
			if err != nil {
				t.Fatal(err)
			}
			var got map[string]string
			if err := json.Unmarshal(data, &got); err != nil {
				t.Fatal(err)
			}
			if got["tls"] != "true" || got["calls"] == "0" {
				t.Fatalf("TLS or transport setup failed: %s", data)
			}
			bad := name == "truncated" || name == "read-error" || name == "timeout" || name == "oversize"
			if bad {
				if got["error"] == "" {
					t.Fatalf("incomplete or failed response accepted: %s", data)
				}
				return
			}
			want := "abcdef"
			if name == "binary" {
				want = "a\x00b"
			}
			if name == "head" || name == "head-chunked" || name == "head-split" {
				want = ""
			}
			if got["status"] != "200" || got["error"] != "" || got["body"] != want {
				t.Fatalf("response truncated: %s", data)
			}
		})
	}
}
