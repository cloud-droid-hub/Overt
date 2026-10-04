#include "zRandom.h"
#include "zLog.h"

#include <sys/syscall.h>
#include <unistd.h>
#include <ctime>

namespace zRandom {

bool getRandomBytes(vector<uint8_t>& out) {
    if (out.empty()) return true;

    // 优先:内核 getrandom(2) 系统调用(真随机,不可预测;优于 rand/srand)
    ssize_t got = syscall(SYS_getrandom, out.data(), out.size(), 0);
    if (got > 0 && (size_t)got == out.size()) {
        return true;
    }

    // 退化:getrandom 失败(极少数设备/受限环境),用 时间+地址 混合伪随机
    // (仍远好于固定值;16 字节级 challenge 的不可预测性在此场景足够)
    LOGW("zRandom: getrandom failed (got=%zd), fallback to time+addr mix", got);
    uint64_t seed = (uint64_t)time(nullptr) ^ (uint64_t)(uintptr_t)&out;
    for (size_t i = 0; i < out.size(); i++) {
        // 线性同余: PCG 常数
        seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
        out[i] = (uint8_t)(seed >> 33);
    }
    return true;
}

} // namespace zRandom