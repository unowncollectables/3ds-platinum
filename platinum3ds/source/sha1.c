#include "sha1.h"
#include <string.h>
#define R(x, k) (((x) << (k)) | ((x) >> (32 - (k))))
static void blk(uint32_t *s, const uint8_t *p) {
    uint32_t w[80], a = s[0], b = s[1], c = s[2], d = s[3], e = s[4];
    for (int i = 0; i < 16; i++) w[i] = (uint32_t)p[i*4] << 24 | p[i*4+1] << 16 | p[i*4+2] << 8 | p[i*4+3];
    for (int i = 16; i < 80; i++) w[i] = R(w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16], 1);
    for (int i = 0; i < 80; i++) {
        uint32_t f, k;
        if (i < 20) { f = (b & c) | (~b & d); k = 0x5A827999; } else if (i < 40) { f = b ^ c ^ d; k = 0x6ED9EBA1; }
        else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDC; } else { f = b ^ c ^ d; k = 0xCA62C1D6; }
        uint32_t t = R(a, 5) + f + e + k + w[i]; e = d; d = c; c = R(b, 30); b = a; a = t;
    }
    s[0] += a; s[1] += b; s[2] += c; s[3] += d; s[4] += e;
}
void SHA1Init(SHA1_CTX *c) { static const uint32_t i[5] = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0}; memcpy(c->s, i, 20); c->n = 0; }
void SHA1Update(SHA1_CTX *c, const uint8_t *d, size_t len) {
    size_t f = c->n % 64; c->n += len;
    if (f) { size_t t = 64 - f; if (t > len) t = len; memcpy(c->b + f, d, t); d += t; len -= t; if (f + t < 64) return; blk(c->s, c->b); }
    for (; len >= 64; d += 64, len -= 64) blk(c->s, d);
    memcpy(c->b, d, len);
}
void SHA1Final(uint8_t out[20], SHA1_CTX *c) {
    uint64_t bits = c->n * 8; uint8_t pad = 0x80, z = 0; SHA1Update(c, &pad, 1);
    while (c->n % 64 != 56) SHA1Update(c, &z, 1);
    uint8_t L[8]; for (int i = 0; i < 8; i++) L[i] = (uint8_t)(bits >> (56 - 8 * i)); SHA1Update(c, L, 8);
    for (int i = 0; i < 20; i++) out[i] = (uint8_t)(c->s[i / 4] >> (24 - 8 * (i % 4)));
}
