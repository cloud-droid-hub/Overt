// Copyright (c) 2025-2026 feicong(https://github.com/feicong/feicong-course)
package ssltest

import (
	"context"
	"encoding/json"
	"os"
	"os/exec"
	"strconv"
	"strings"
	"testing"
	"time"
)

func TestIPC(t *testing.T) {
	adb := os.Getenv("CDH_ADB")
	serial := os.Getenv("CDH_ADB_SERIAL")
	if adb == "" || serial == "" {
		t.Fatal("explicit adb and serial required; launch the tested app through its UI first")
	}
	run := func(args ...string) (string, error) {
		ctx, cancel := context.WithTimeout(context.Background(), 3*time.Second)
		defer cancel()
		data, err := exec.CommandContext(ctx, adb, append([]string{"-s", serial}, args...)...).CombinedOutput()
		return strings.TrimSpace(string(data)), err
	}
	if uid, err := run("shell", "id", "-u"); err != nil || uid != "2000" {
		t.Fatalf("shell UID=%s: %v", uid, err)
	}
	if mode, err := run("shell", "getenforce"); err != nil || mode != "Enforcing" {
		t.Fatalf("SELinux=%s: %v", mode, err)
	}
	main, err := run("shell", "pidof", "com.example.overt")
	if err != nil || main == "" {
		t.Fatalf("main app must already be running: %s %v", main, err)
	}
	t.Logf("main=%s", main)
	end := time.Now().Add(15 * time.Second)
	last := ""
	for {
		data, err := run("shell", "ps", "-A", "-o", "PID,UID,NAME")
		if err != nil {
			t.Fatalf("process list: %s %v", data, err)
		}
		for _, line := range strings.Split(data, "\n") {
			row := strings.Fields(line)
			if len(row) != 3 || row[2] != "com.example.overt:overt_server_iso:com.example.overt.Server" {
				continue
			}
			uid, err := strconv.Atoi(row[1])
			if err != nil || uid < 90000 || uid > 99999 {
				t.Fatalf("isolated UID invalid: %s %v", line, err)
			}
			t.Logf("isolated PID, UID, name=%s", line)
			data, err := run("shell", "run-as", "com.example.overt", "cat", "files/overt-cards/isoloated_process_info.json")
			var raw map[string]struct{ Value string }
			if err == nil && json.Unmarshal([]byte(data), &raw) == nil &&
				raw["libc.so"].Value != "" && raw["mountinfo.self"].Value != "" {
				t.Logf("complete JSON: bytes=%d, raw items=%d", len(data), len(raw))
				return
			}
			last = "isolated process exists but complete raw JSON is unavailable"
		}
		if time.Now().After(end) {
			t.Fatalf("isolated process or complete reply missing for 15s; main PID=%s, detail=%s", main, last)
		}
		time.Sleep(500 * time.Millisecond)
	}
}
