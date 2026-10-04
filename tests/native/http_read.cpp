// Copyright (c) 2025-2026 fei_cong(https://github.com/feicong/feicong-course)
#include "zHttps.h"
#include "zJson.h"
#include <cstdio>

static vector<string> pieces;
static size_t step;
static size_t offset;
static int calls;
static int end_code;

void zLogPrint(int level, const char* tag, const char* file, const char* func,
			   int line, const char* fmt, ...) {}

extern "C" int ssl_read_wrap(mbedtls_ssl_context* ssl, unsigned char* buf,
							 size_t size) __asm__("__wrap_mbedtls_ssl_read");

extern "C" int ssl_read_wrap(mbedtls_ssl_context* ssl,
									 unsigned char* buf, size_t size) {
	++calls;
	if (step == pieces.size()) return end_code;
	const auto& part = pieces[step];
	size_t count = part.size() - offset;
	if (count > size) count = size;
	memcpy(buf, part.data() + offset, count);
	offset += count;
	if (offset == part.size()) {
		++step;
		offset = 0;
	}
	return static_cast<int>(count);
}

int main(int argc, char** argv) {
	if (argc != 2) return 2;
	string name = argv[1];
	string header = "HTTP/1.1 200 OK\r\nContent-Length: 6\r\nConnection: close\r\n\r\n";
	if (name == "split-close") pieces = {header, "abc", "def"};
	else if (name == "many-records") pieces = {header, "a", "b", "c", "d", "e", "f"};
	else if (name == "length-last") pieces = {"HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: 6\r\n\r\n", "abc", "def"};
	else if (name == "until-eof") pieces = {"HTTP/1.1 200 OK\r\nConnection: close\r\n\r\n", "abc", "def"};
	else if (name == "chunked") pieces = {"HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\nConnection: close\r\n\r\n", "3\r\n", "abc\r\n", "3\r\n", "def\r\n", "0\r\n\r\n"};
	else if (name == "truncated") pieces = {header, "abc"};
	else if (name == "read-error") {
		pieces = {header};
		end_code = MBEDTLS_ERR_NET_RECV_FAILED;
	} else if (name == "timeout") {
		pieces = {header};
		end_code = MBEDTLS_ERR_SSL_TIMEOUT;
	} else if (name == "oversize") pieces = {"HTTP/1.1 200 OK\r\nContent-Length: 70000\r\nConnection: close\r\n\r\n", string(70000, 'x')};
	else if (name == "binary") pieces = {"HTTP/1.1 200 OK\r\nContent-Length: 3\r\nConnection: close\r\n\r\n", string("a\0b", 3)};
	else return 2;

	// TCP、TLS协商与证书校验实际执行，只控制响应读取的分段边界。
	zHttps client(12);
	HttpsRequest req("https://r.inews.qq.com/api/ip2city", "GET", 12);
	auto reply = client.performRequest(req);
	map<string, string> data;
	data["tls"] = reply.ssl_verification_passed ? "true" : "false";
	data["status"] = to_string(reply.status_code);
	data["error"] = reply.error_message;
	data["body"] = reply.body;
	data["calls"] = to_string(calls);
	zJson out = data;
	puts(out.dump().c_str());
	return fflush(stdout) == 0 ? 0 : 4;
}
