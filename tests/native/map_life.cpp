// Copyright (c) 2025-2026 fei_cong(https://github.com/feicong/feicong-course)
#include "zProcMaps.h"
#include <cstdio>

void zLogPrint(int level, const char* tag, const char* file, const char* func,
			   int line, const char* fmt, ...) {}

int main() {
	zProcMaps maps;
	string path = "/data/local/tmp/map-case/oat/arm64/base.odex";
	LibraryMapping item{};
	item.file_path = path;
	maps.loaded_libraries[path] = item;
	auto* got = maps.find_so_by_name("/oat/arm64/base.odex");
	auto* stored = &maps.loaded_libraries.at(path);
	bool stable = got == stored;
	bool missing = maps.find_so_by_name("/missing/map-case.so") == nullptr;
	bool changed = false;
	if (stable) {
		got->inode = "219";
		changed = maps.loaded_libraries.at(path).inode == "219";
	}
	printf("{\"stable\":%s,\"missing\":%s,\"changed\":%s}\n",
		   stable ? "true" : "false", missing ? "true" : "false",
		   changed ? "true" : "false");
	return fflush(stdout) == 0 ? 0 : 4;
}
