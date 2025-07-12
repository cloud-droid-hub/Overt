//
// Created by lxz on 2025/7/12.
//

#ifndef OVERT_CRC_H
#define OVERT_CRC_H


#include <stddef.h>
#include <stdint.h>

uint32_t crc32c_fold(const void *data, size_t len);

#endif //OVERT_CRC_H
