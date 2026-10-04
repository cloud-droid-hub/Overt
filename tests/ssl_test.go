// Copyright (c) 2025-2026 fei_cong(https://github.com/feicong/feicong-course)
package ssltest

import (
	"encoding/json"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"testing"
)

type sslCase struct {
	name   string
	body   string
	pin    string
	tls    string
	status string
	error  string
	risk   string
	text   string
}

const leaf = "EA725FF9B6B1A8D8A823A6DE0C59A24496FC38E937C03EF6F5D84C66F107C421"

func shellArg(s string) string {
	return "'" + strings.ReplaceAll(s, "'", "'\\''") + "'"
}

func prepSSL(t *testing.T) (string, string) {
	t.Helper()
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
	bin := filepath.Join(out, "overt-ssl-check")
	args := []string{"-std=c++17", "-O0", "-static-libstdc++"}
	for _, part := range []string{"zconfig", "zlog", "zlibc", "zstd", "zcore", "zinfo"} {
		args = append(args, "-I"+filepath.Join(src, part, "src/main/cpp"))
	}
	args = append(args, "-I"+filepath.Join(src, "zcore/src/main/cpp/include"),
		filepath.Join(src, "tests/native/ssl_reply.cpp"),
		filepath.Join(src, "zinfo/src/main/cpp/zSslInfo.cpp"),
		filepath.Join(src, "zlibc/src/main/cpp/zLibc.cpp"), "-o", bin)
	cmd := exec.Command(tool, args...)
	cmd.Env = append(os.Environ(), "TMPDIR="+out, "TMP="+out, "TEMP="+out, "GOTMPDIR="+out)
	if data, err := cmd.CombinedOutput(); err != nil {
		t.Fatalf("native compile: %s %v", data, err)
	}
	guest := "/data/local/tmp/overt-ssl-check"
	for _, args := range [][]string{{"push", bin, guest}, {"shell", "chmod", "0755", guest}} {
		if data, err := exec.Command(adb, append([]string{"-s", serial}, args...)...).CombinedOutput(); err != nil {
			t.Fatalf("native transfer: %s %v", data, err)
		}
	}
	return guest, serial
}

func TestSSLReply(t *testing.T) {
	bin, serial := prepSSL(t)
	cn := `{"ret":0,"country":"中国","province":"浙江省","city":"杭州市"}`
	us := `{"ret":0,"country":"美国","province":"弗吉尼亚州","city":"阿什本"}`
	cases := []sslCase{
		{"cn", cn, leaf, "1", "200", "", "safe", "中国浙江省杭州市"},
		{"us", us, leaf, "1", "200", "", "safe", "美国弗吉尼亚州阿什本"},
		{"same-city", `{"ret":0,"country":"中国","province":"上海市","city":"上海市"}`, leaf, "1", "200", "", "safe", "中国上海市"},
		{"country-only", `{"ret":0,"country":"日本"}`, leaf, "1", "200", "", "safe", "日本"},
		{"bad-pin", cn, "00", "1", "200", "", "error", "get_location failed"},
		{"old-pin", cn, "A58095F1C26CA01A5AAC2666DCAA66182BE423BE47973BBD1F3CCFF9ACA59D14", "1", "200", "", "error", "get_location failed"},
		{"bad-tls", cn, leaf, "0", "200", "", "error", "get_location failed"},
		{"http-error", cn, leaf, "1", "503", "", "error", "get_location failed"},
		{"transport-error", cn, leaf, "1", "200", "connection failed", "error", "get_location failed"},
		{"ret-error", `{"ret":1,"country":"中国"}`, leaf, "1", "200", "", "error", "get_location failed"},
		{"missing-ret", `{"country":"中国"}`, leaf, "1", "200", "", "error", "get_location failed"},
		{"float-ret", `{"ret":0.0,"country":"中国"}`, leaf, "1", "200", "", "error", "get_location failed"},
		{"large-ret", `{"ret":4294967296,"country":"中国"}`, leaf, "1", "200", "", "error", "get_location failed"},
		{"empty-country", `{"ret":0,"country":""}`, leaf, "1", "200", "", "error", "get_location failed"},
		{"blank-country", `{"ret":0,"country":"   "}`, leaf, "1", "200", "", "error", "get_location failed"},
		{"null-country", `{"ret":0,"country":null}`, leaf, "1", "200", "", "error", "get_location failed"},
		{"wrong-country", `{"ret":0,"country":7}`, leaf, "1", "200", "", "error", "get_location failed"},
		{"wrong-province", `{"ret":0,"country":"中国","province":7}`, leaf, "1", "200", "", "error", "get_location failed"},
		{"wrong-city", `{"ret":0,"country":"中国","city":null}`, leaf, "1", "200", "", "error", "get_location failed"},
		{"control", `{"ret":0,"country":"中国\u0000"}`, leaf, "1", "200", "", "error", "get_location failed"},
		{"malformed", `{`, leaf, "1", "200", "", "error", "get_location failed"},
		{"array", `[]`, leaf, "1", "200", "", "error", "get_location failed"},
	}
	for _, c := range cases {
		t.Run(c.name, func(t *testing.T) {
			parts := []string{bin, c.body, c.pin, c.tls, c.status, c.error, c.name}
			for i := range parts {
				parts[i] = shellArg(parts[i])
			}
			args := []string{"-s", serial, "shell", strings.Join(parts, " ")}
			cmd := exec.Command(os.Getenv("CDH_ADB"), args...)
			data, err := cmd.Output()
			t.Logf("native response: %s error=%v", data, err)
			if err != nil {
				if fail, ok := err.(*exec.ExitError); ok {
					t.Logf("native stderr: %s", fail.Stderr)
				}
				t.FailNow()
			}
			var got map[string]map[string]string
			if err := json.Unmarshal(data, &got); err != nil {
				t.Fatalf("native JSON: %s %v", data, err)
			}
			row := got["location"]
			if row["risk"] != c.risk || row["explain"] != c.text || len(got) != 1 {
				t.Fatalf("response=%s, want=%s/%s", data, c.risk, c.text)
			}
		})
	}
}
