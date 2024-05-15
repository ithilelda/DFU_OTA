/*
 * crc.h
 *
 *  Created on: May 7, 2024
 *      Author: huawe
 */

#ifndef CRC_H
#define CRC_H

#include "config.h"


#define CRC_INITIAL_VALUE           0
#define CRC_XOROT                   0xFFFFFFFF
#define CRC_REFLECT_DATA            0
#define CRC_REFLECT_REMAINDER       0
#define CRC_REMAINDER               0xFFFFFFFF
#define CRC_POLYNOMIAL              0x04C11DB7


uint32_t update_CRC32 (uint32_t crc32, void *pStart, uint32_t uSize);
uint32_t calculate_CRC32 (void *pStart, uint32_t uSize);

#endif /* CRC_H */
