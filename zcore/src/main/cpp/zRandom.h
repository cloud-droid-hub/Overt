#ifndef OVERT_ZRANDOM_H
#define OVERT_ZRANDOM_H

#include "zStd.h"
#include <vector>

/**
 * 通用随机数工具(zcore 基础库)。
 * 用于生成不可预测的随机字节(如 attestation challenge、nonce 等)。
 */
namespace zRandom {

/**
 * 生成安全随机字节。
 * 优先使用内核 getrandom(2) 系统调用(真随机,不可预测);
 * 极少数设备 getrandom 失败时退化为 时间+地址 混合伪随机(尽力而为)。
 * @param out 输出缓冲区(由调用方 resize 指定长度)
 * @return true=成功(已填充), false=全部失败(缓冲区内容不可用)
 */
bool getRandomBytes(vector<uint8_t>& out);

} // namespace zRandom

#endif //OVERT_ZRANDOM_H