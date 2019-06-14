#ifndef _MURMURHASH3_H_
#define _MURMURHASH3_H_
#include <stdint.h>

uint32_t MurmurHash3_32(const void *key, int len, uint32_t seed);

#endif  // #ifndef _MURMURHASH3_H_
