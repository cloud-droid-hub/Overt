// Copyright (c) 2025-2026 feicong(https://github.com/feicong/feicong-course)
#include "zTimeInfo.h"
#include "zHttps.h"
#include "zJson.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

#if !defined(CDH_TIME_LIVE)
namespace {
    string body;
    string pin;
    string error;
    bool verified;
    int status;
}

zHttps::zHttps(int) {}
zHttps::~zHttps() {}

void HttpsRequest::parseUrl() {
    is_https = true;
    host = "api.pinduoduo.com";
}

HttpsResponse zHttps::performRequest(const HttpsRequest &) {
    HttpsResponse reply;
    reply.body = body;
    reply.error_message = error;
    reply.status_code = status;
    reply.ssl_verification_passed = verified;
    reply.certificate.fingerprint_sha256 = pin;
    return reply;
}
#endif

void zLogPrint(int, const char *, const char *, const char *, int, const char *, ...) {}

int main(int argc, char **argv) {
#if defined(CDH_TIME_LIVE)
    zHttps client(10);
    HttpsRequest req("https://api.pinduoduo.com/api/server/_stm", "GET", 10);
    const auto reply = client.performRequest(req);
    map<string, string> fields;
    fields["tls"] = reply.ssl_verification_passed ? "true" : "false";
    fields["status"] = to_string(reply.status_code);
    fields["pin"] = reply.certificate.fingerprint_sha256;
    fields["error"] = reply.error_message;
    fields["body"] = reply.body;
    const auto info = get_time_info();
    fields["remote"] = info.at("remote_current_time").at("value");
    fields["local"] = info.at("local_current_time").at("value");
    zJson out = fields;
#else
    if (argc != 6) return 2;
    body = argv[1];
    pin = argv[2];
    verified = strcmp(argv[3], "1") == 0;
    status = atoi(argv[4]);
    error = argv[5];
    zJson out = get_time_info();
#endif
    puts(out.dump().c_str());
    return fflush(stdout) == 0 ? 0 : 3;
}
