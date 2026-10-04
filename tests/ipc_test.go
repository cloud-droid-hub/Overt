// Copyright (c) 2025-2026 feicong(https://github.com/feicong/feicong-course)
package ssltest

import (
	"context"
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
	for {
		pid, err := run("shell", "pidof", "com.example.overt:overt_server_iso")
		if err == nil && pid != "" {
			if len(strings.Fields(pid)) != 1 {
				t.Fatalf("unexpected isolated PID list: %s", pid)
			}
			data, err := run("shell", "ps", "-p", pid, "-o", "UID,NAME")
			if err != nil {
				t.Fatalf("isolated process: %s %v", data, err)
			}
			t.Log(data)
			lines := strings.Split(data, "\n")
			if len(lines) != 2 {
				t.Fatalf("unexpected process output: %s", data)
			}
			row := strings.Fields(lines[1])
			if len(row) != 2 {
				t.Fatalf("unexpected isolated row: %s", data)
			}
			uid, err := strconv.Atoi(row[0])
			if err != nil || uid < 90000 || uid > 99999 || row[1] != "com.example.overt:overt_server_iso" {
				t.Fatalf("isolated UID/name invalid: %s %v", data, err)
			}
			return
		}
		if time.Now().After(end) {
			t.Fatalf("isolated process missing for 15s; main PID=%s, pidof=%q: %v", main, pid, err)
		}
		time.Sleep(500 * time.Millisecond)
	}
}
