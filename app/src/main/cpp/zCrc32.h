//
// Created by lxz on 2025/7/12.
//

#ifndef OVERT_ZCRC32_H
#define OVERT_ZCRC32_H


#include <stddef.h>
#include <stdint.h>
#include "zLog.h"

uint32_t crc32c_fold(const void *data, size_t len);

#endif //OVERT_ZCRC32_H
