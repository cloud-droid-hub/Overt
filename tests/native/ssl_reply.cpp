// Copyright (c) 2025-2026 fei_cong(https://github.com/feicong/feicong-course)
#include "zSslInfo.h"
#include "zHttps.h"
#include "zJson.h"
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstring>

static string body;
static string pin;
static string error;
static bool verified;
static int status;

// 受控响应只替换网络边界，规则使用实际实现及非标准容器。
zHttps::zHttps(int seconds) {}
zHttps::~zHttps() {}

void HttpsRequest::parseUrl() {
	is_https = true;
	host = url.find("baidu.com") != string::npos ? "www.baidu.com" : "r.inews.qq.com";
}

HttpsResponse zHttps::performRequest(const HttpsRequest& req) {
	HttpsResponse reply;
	if (req.host == "www.baidu.com") {
		reply.status_code = 200;
		reply.ssl_verification_passed = true;
		reply.certificate.fingerprint_sha256 = "CA5688C552685190E98B94C40E94F842EE7FDA39B08846FBD4D7E2ED7211B4F2";
	} else {
		reply.body = body;
		reply.error_message = error;
		reply.status_code = status;
		reply.ssl_verification_passed = verified;
		reply.certificate.fingerprint_sha256 = pin;
	}
	return reply;
}

void zLogPrint(int level, const char* tag, const char* file, const char* func,
			   int line, const char* fmt, ...) {
	// 测试不写检测日志，避免宿主输出函数再次调用被测非标准接口。
}

bool string_start_with(const char* text, const char* prefix) {
	return strncmp(text, prefix, strlen(prefix)) == 0;
}

int main(int argc, char** argv) {
	if (argc != 7) return 2;
	body = argv[1];
	pin = argv[2];
	verified = strcmp(argv[3], "1") == 0;
	status = atoi(argv[4]);
	error = argv[5];
	try {
		auto info = get_ssl_info();
		zJson out = info;
		puts(out.dump().c_str());
		if (fflush(stdout) != 0) return 4;
		return 0;
	} catch (const zJson::exception& err) {
		fprintf(stderr, "uncaught response exception: %s\n", err.what());
		return 3;
	}
}
