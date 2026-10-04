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
		{"cn", cn, leaf, "1", "200", "", "中国浙江省杭州市"},
		{"us", us, leaf, "1", "200", "", "美国弗吉尼亚州阿什本"},
		{"same", `{"ret":0,"country":"中国","province":"上海市","city":"上海市"}`, leaf, "1", "200", "", "中国上海市"},
		{"ctry", `{"ret":0,"country":"日本"}`, leaf, "1", "200", "", "日本"},
		{"pin", cn, "00", "1", "200", "", "中国浙江省杭州市"},
		{"old", cn, "A58095F1C26CA01A5AAC2666DCAA66182BE423BE47973BBD1F3CCFF9ACA59D14", "1", "200", "", "中国浙江省杭州市"},
		{"tls", cn, leaf, "0", "200", "", ""},
		{"http", cn, leaf, "1", "503", "", ""},
		{"tran", cn, leaf, "1", "200", "connection failed", ""},
		{"ret", `{"ret":1,"country":"中国"}`, leaf, "1", "200", "", ""},
		{"miss", `{"country":"中国"}`, leaf, "1", "200", "", ""},
		{"flt", `{"ret":0.0,"country":"中国"}`, leaf, "1", "200", "", ""},
		{"big", `{"ret":4294967296,"country":"中国"}`, leaf, "1", "200", "", ""},
		{"empt", `{"ret":0,"country":""}`, leaf, "1", "200", "", ""},
		{"spc", `{"ret":0,"country":"   "}`, leaf, "1", "200", "", ""},
		{"null", `{"ret":0,"country":null}`, leaf, "1", "200", "", ""},
		{"num", `{"ret":0,"country":7}`, leaf, "1", "200", "", ""},
		{"prov", `{"ret":0,"country":"中国","province":7}`, leaf, "1", "200", "", ""},
		{"city", `{"ret":0,"country":"中国","city":null}`, leaf, "1", "200", "", ""},
		{"ctl", `{"ret":0,"country":"中国\u0000"}`, leaf, "1", "200", "", ""},
		{"json", `{`, leaf, "1", "200", "", ""},
		{"arr", `[]`, leaf, "1", "200", "", ""},
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
			qq := got["https://r.inews.qq.com/api/ip2city"]
			wantErr := c.error
			if wantErr == "" && c.tls == "0" {
				wantErr = "certificate verification failed"
			}
			if wantErr == "" && c.status != "200" {
				wantErr = "unexpected HTTP status"
			}
			if row["value"] != c.text || len(row) != 1 || len(got) != 3 ||
				qq["value"] != c.pin || qq["error"] != wantErr || len(qq) != 2 {
				t.Fatalf("response=%s, want location=%s pin=%s error=%s", data, c.text, c.pin, wantErr)
			}
		})
	}
}
