/*
Copyright 2019 Tinic Uro

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include "./murmur_hash3.h"

static uint32_t rotl32 ( uint32_t x, int8_t r ) { 
    return (x << r) | (x >> (32 - r));
}

#define ROTL32(x,y) rotl32(x,y)

static uint32_t fmix32 ( uint32_t h ) {
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}

uint32_t MurmurHash3_32(const void *key, int len, uint32_t seed) {
    const uint8_t * data = reinterpret_cast<const uint8_t *>(key);
    const int nblocks = len / 4;

    uint32_t h1 = seed;

    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;

    const uint32_t *blocks = reinterpret_cast<const uint32_t *>(data + nblocks*4);

    for(int i = -nblocks; i; i++) {
        uint32_t k1 = blocks[i];

        k1 *= c1;
        k1 = ROTL32(k1,15);
        k1 *= c2;

        h1 ^= k1;
        h1 = ROTL32(h1,13); 
        h1 = h1*5+0xe6546b64;
    }

    const uint8_t *tail = reinterpret_cast<const uint8_t *>(data + nblocks*4);

    uint32_t k1 = 0;

    switch(len & 3) {
    case 3: k1 ^= tail[2] << 16; // fallthrough
    case 2: k1 ^= tail[1] << 8; // fallthrough
    case 1: k1 ^= tail[0];
          k1 *= c1; k1 = ROTL32(k1,15); k1 *= c2; h1 ^= k1;
    };

    h1 ^= len;

    h1 = fmix32(h1);

    return h1;
} 
