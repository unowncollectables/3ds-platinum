#pragma once
#include <stdint.h>
#include <stddef.h>
typedef struct { uint32_t s[5]; uint64_t n; uint8_t b[64]; } SHA1_CTX;
void SHA1Init(SHA1_CTX *c); void SHA1Update(SHA1_CTX *c, const uint8_t *d, size_t len); void SHA1Final(uint8_t out[20], SHA1_CTX *c);
