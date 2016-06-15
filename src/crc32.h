/*
 * crc32.h
 *
 *  Created on: Jun 8, 2016
 *      Author: michele
 */

#ifndef CRC32_H_
#define CRC32_H_
#include <stdint.h>
uint32_t crc32(uint32_t crc, const void *buf, size_t size);

#endif /* CRC32_H_ */
