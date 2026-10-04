#include "zBase64.h"

namespace zBase64 {

namespace {

const char* const kTable = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// 反查表:字符 → 6bit 值(非法为 -1)
int decode_val(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

} // namespace

string encode(const uint8_t* data, size_t len) {
    string out;
    if (!data) return out;
    out.reserve(((len + 2) / 3) * 4);

    size_t i = 0;
    while (i + 3 <= len) {
        uint32_t n = (uint32_t(data[i]) << 16) | (uint32_t(data[i+1]) << 8) | uint32_t(data[i+2]);
        out += kTable[(n >> 18) & 0x3F];
        out += kTable[(n >> 12) & 0x3F];
        out += kTable[(n >> 6) & 0x3F];
        out += kTable[n & 0x3F];
        i += 3;
    }

    size_t rem = len - i;
    if (rem == 1) {
        uint32_t n = uint32_t(data[i]) << 16;
        out += kTable[(n >> 18) & 0x3F];
        out += kTable[(n >> 12) & 0x3F];
        out += "==";
    } else if (rem == 2) {
        uint32_t n = (uint32_t(data[i]) << 16) | (uint32_t(data[i+1]) << 8);
        out += kTable[(n >> 18) & 0x3F];
        out += kTable[(n >> 12) & 0x3F];
        out += kTable[(n >> 6) & 0x3F];
        out += "=";
    }
    return out;
}

string encode(const vector<uint8_t>& data) {
    return encode(data.data(), data.size());
}

bool decode(const string& b64, vector<uint8_t>& out) {
    out.clear();
    // 去空白
    string clean;
    clean.reserve(b64.size());
    for (char c : b64) {
        if (c == ' ' || c == '\n' || c == '\r' || c == '\t') continue;
        clean += c;
    }
    if (clean.size() % 4 != 0) return false;

    // 处理可能的 = 填充
    size_t padding = 0;
    if (!clean.empty() && clean[clean.size()-1] == '=') padding++;
    if (clean.size() > 1 && clean[clean.size()-2] == '=') padding++;
    if (padding > 2) return false;
    size_t data_len = (clean.size() / 4) * 3 - padding;
    if (data_len == 0 && clean.size() == 0) return true;

    out.reserve(data_len);
    for (size_t i = 0; i < clean.size(); i += 4) {
        int v0 = decode_val(clean[i]);
        int v1 = decode_val(clean[i+1]);
        int v2 = (i + 2 < clean.size() && clean[i+2] != '=') ? decode_val(clean[i+2]) : -1;
        int v3 = (i + 3 < clean.size() && clean[i+3] != '=') ? decode_val(clean[i+3]) : -1;
        if (v0 < 0 || v1 < 0) return false;
        uint32_t n = (uint32_t(v0) << 18) | (uint32_t(v1) << 12);
        if (v2 >= 0) n |= (uint32_t(v2) << 6);
        if (v3 >= 0) n |= uint32_t(v3);
        out.push_back((n >> 16) & 0xFF);
        if (v2 >= 0) out.push_back((n >> 8) & 0xFF);
        if (v3 >= 0) out.push_back(n & 0xFF);
    }
    // 校验长度一致
    if (out.size() != data_len) {
        // 部分非法填充 case:截断到预期长度(大多实现容忍)
        out.resize(data_len);
    }
    return true;
}

} // namespace zBase64