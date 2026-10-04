// Copyright (c) 2025-2026 fei_cong(https://github.com/feicong/feicong-course)
#include "zProcMaps.h"
#include <cstdio>

void zLogPrint(int level, const char* tag, const char* file, const char* func,
			   int line, const char* fmt, ...) {}

int main() {
	string path = "/data/local/tmp/map-case/oat/arm64/base.odex";
	LibraryMapping got{};
	bool copied = false;
	bool missing = false;
	bool isolated = false;
	{
		zProcMaps maps;
		LibraryMapping item{};
		item.file_path = path;
		maps.loaded_libraries[path] = item;
		got = maps.find_so_by_name("/oat/arm64/base.odex");
		copied = got.file_path == path && &got != &maps.loaded_libraries.at(path);
		auto absent = maps.find_so_by_name("/missing/map-case.so");
		missing = absent.file_path.empty() && absent.address_range_start == nullptr;
		got.inode = "219";
		isolated = maps.loaded_libraries.at(path).inode != "219";
	}
	bool alive = got.file_path == path && got.inode == "219";
	printf("{\"copy\":%s,\"missing\":%s,\"isolated\":%s,\"alive\":%s}\n",
		   copied ? "true" : "false", missing ? "true" : "false",
		   isolated ? "true" : "false", alive ? "true" : "false");
	return fflush(stdout) == 0 ? 0 : 4;
}
