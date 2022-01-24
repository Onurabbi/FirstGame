#ifndef HASH_H
#define HASH_H

#include <string.h>
#include <stdint.h>

#define  SEED 1234

uint32_t murmur3_32(const uint8_t* key, size_t len, uint32_t seed);

#endif