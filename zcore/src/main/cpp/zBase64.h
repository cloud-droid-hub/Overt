#ifndef OVERT_ZBASE64_H
#define OVERT_ZBASE64_H

#include "zStd.h"
#include <vector>

/**
 * 通用 base64 编解码工具(zcore 基础库)。
 * 用于将二进制数据(DER 证书链等)编码为 JSON 可传输的字符串。
 */
namespace zBase64 {

/**
 * 标准 base64 编码(RFC 4648, 含 = 填充)。
 * @param data 输入字节
 * @return base64 字符串
 */
string encode(const vector<uint8_t>& data);

/**
 * 标准 base64 编码(纯字节指针输入)。
 */
string encode(const uint8_t* data, size_t len);

/**
 * base64 解码(RFC 4648, 容忍 = 填充)。
 * @param b64 输入 base64 字符串
 * @param out 解码输出字节(成功时填充)
 * @return true=成功, false=输入非法
 */
bool decode(const string& b64, vector<uint8_t>& out);

} // namespace zBase64

#endif //OVERT_ZBASE64_H