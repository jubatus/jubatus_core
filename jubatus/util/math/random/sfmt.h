// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2016 Preferred Networks and Nippon Telegraph and Telephone Corporation.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License version 2.1 as published by the Free Software Foundation.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

/*
SFMT implementation is based official code.
(http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/SFMT/index.html)

Changes:
* uses c++ template to switch sfmt parameters
* remove non-SSE2, Big-Endian code

Copyright (c) 2006,2007 Mutsuo Saito, Makoto Matsumoto and Hiroshima
University.
Copyright (c) 2012 Mutsuo Saito, Makoto Matsumoto, Hiroshima University
and The University of Tokyo.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the names of Hiroshima University, The University of
      Tokyo nor the names of its contributors may be used to endorse
      or promote products derived from this software without specific
      prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef JUBATUS_UTIL_MATH_RANDOM_SFMT_H_
#define JUBATUS_UTIL_MATH_RANDOM_SFMT_H_
#include <cstring>
#include <stdint.h>

#ifdef __SSE2__
#include <emmintrin.h>
#endif

namespace jubatus {
namespace util{
namespace math{
namespace random{

#ifndef __SSE2__
typedef uint32_t __m128i[4];
#endif

template<uint32_t MEXP, int POS1, int SL1, int SL2, int SR1, int SR2,
         uint32_t MSK1, uint32_t MSK2, uint32_t MSK3, uint32_t MSK4,
         uint32_t PARITY1, uint32_t PARITY2, uint32_t PARITY3, uint32_t PARITY4>
class sfmt {
  static const uint32_t N = MEXP / 128 + 1;
  static const uint32_t N32 = N * 4;
  union {
    uint32_t u[4];
    uint64_t u64[2];
    __m128i si;
  } s[N];
  uint64_t idx;

public:
  sfmt(uint32_t seed) : idx(N32) {
    static const uint32_t parity[4] = {PARITY1, PARITY2, PARITY3, PARITY4};
    uint32_t *psfmt32 = &(s[0].u[0]);
    psfmt32[0] = seed;
    for (uint32_t i = 1; i < N32; i++) {
      psfmt32[i] = 1812433253UL * (psfmt32[i - 1] ^ (psfmt32[i - 1] >> 30)) + i;
    }

    //period_certification
    int inner = 0;
    for (int i = 0; i < 4; i++)
      inner ^= psfmt32[i] & parity[i];
    for (int i = 16; i > 0; i >>= 1)
      inner ^= inner >> i;
    inner &= 1;
    if (inner == 1)
      return;
    for (int i = 0; i < 4; i++) {
      int work = 1;
      for (int j = 0; j < 32; j++) {
        if ((work & parity[i]) != 0) {
          psfmt32[i] ^= work;
          return;
        }
        work = work << 1;
      }
    }
  }
  ~sfmt() {}

  uint32_t next() {
    uint32_t *psfmt32 = &(s[0].u[0]);
    if (idx >= N32) {
      generate_all();
      idx = 0;
    }
    return psfmt32[idx++];
  }

  void fill_int_unsafe(uint32_t *out, size_t n) {
    uint32_t *psfmt32 = &(s[0].u[0]);
    if (idx + n <= N32) {
      std::memcpy(out, psfmt32 + idx, n * sizeof(uint32_t));
      idx += n;
    } else {
      while (n > 0) {
        if (idx >= N32) {
          generate_all();
          idx = 0;
        }
        size_t m = (n + idx <= N32 ? n : N32 - idx);
        std::memcpy(out, psfmt32 + idx, m * sizeof(uint32_t));
        idx += m;
        out += m;
        n -= m;
      }
    }
  }
private:

#ifdef __SSE2__

  inline static void mm_recursion(__m128i * r, __m128i a, __m128i b,
                                  __m128i c, __m128i d) {
    static const uint32_t sse2_param_mask[4] __attribute__((aligned(16)))
      = {MSK1, MSK2, MSK3, MSK4};
    __m128i v, x, y, z;
    y = _mm_srli_epi32(b, SR1);
    z = _mm_srli_si128(c, SR2);
    v = _mm_slli_epi32(d, SL1);
    z = _mm_xor_si128(z, a);
    z = _mm_xor_si128(z, v);
    x = _mm_slli_si128(a, SL2);
    y = _mm_and_si128(y, *(const __m128i*)sse2_param_mask);
    z = _mm_xor_si128(z, x);
    z = _mm_xor_si128(z, y);
    *r = z;
  }

  void generate_all() {
    uint32_t i;
    __m128i r1, r2;
    r1 = s[N - 2].si;
    r2 = s[N - 1].si;
    for (i = 0; i < N - POS1; i++) {
      mm_recursion(&s[i].si, s[i].si, s[i + POS1].si, r1, r2);
      r1 = r2;
      r2 = s[i].si;
    }
    for (; i < N; i++) {
      mm_recursion(&s[i].si, s[i].si, s[i + POS1 - N].si, r1, r2);
      r1 = r2;
      r2 = s[i].si;
    }
  }

#else // #ifdef __SSE2__

  inline static void mm_recursion(__m128i * r, __m128i a, __m128i b,
                                  __m128i c, __m128i d) {
    __m128i x, y;
    {
      uint64_t th, tl, oh, ol;
      th = ((uint64_t)a[3] << 32) | ((uint64_t)a[2]);
      tl = ((uint64_t)a[1] << 32) | ((uint64_t)a[0]);
      oh = th << (SL2 * 8);
      ol = tl << (SL2 * 8);
      oh |= tl >> (64 - SL2 * 8);
      x[1] = (uint32_t)(ol >> 32);
      x[0] = (uint32_t)ol;
      x[3] = (uint32_t)(oh >> 32);
      x[2] = (uint32_t)oh;
    }
    {
      uint64_t th, tl, oh, ol;
      th = ((uint64_t)c[3] << 32) | ((uint64_t)c[2]);
      tl = ((uint64_t)c[1] << 32) | ((uint64_t)c[0]);
      oh = th >> (SR2 * 8);
      ol = tl >> (SR2 * 8);
      ol |= th << (64 - SR2 * 8);
      y[1] = (uint32_t)(ol >> 32);
      y[0] = (uint32_t)ol;
      y[3] = (uint32_t)(oh >> 32);
      y[2] = (uint32_t)oh;
    }
    (*r)[0] = a[0] ^ x[0] ^ ((b[0] >> SR1) & MSK1) ^ y[0] ^ (d[0] << SL1);
    (*r)[1] = a[1] ^ x[1] ^ ((b[1] >> SR1) & MSK2) ^ y[1] ^ (d[1] << SL1);
    (*r)[2] = a[2] ^ x[2] ^ ((b[2] >> SR1) & MSK3) ^ y[2] ^ (d[2] << SL1);
    (*r)[3] = a[3] ^ x[3] ^ ((b[3] >> SR1) & MSK4) ^ y[3] ^ (d[3] << SL1);
  }

  static inline void memcpy_m128i(__m128i dst, const __m128i src) {
    dst[0] = src[0]; dst[1] = src[1];
    dst[2] = src[2]; dst[3] = src[3];
  }

  void generate_all() {
    uint32_t i;
    __m128i r1, r2;
    memcpy_m128i(r1, s[N - 2].si);
    memcpy_m128i(r2, s[N - 1].si);
    for (i = 0; i < N - POS1; i++) {
      mm_recursion(&s[i].si, s[i].si, s[i + POS1].si, r1, r2);
      memcpy_m128i(r1, r2);
      memcpy_m128i(r2, s[i].si);
    }
    for (; i < N; i++) {
      mm_recursion(&s[i].si, s[i].si, s[i + POS1 - N].si, r1, r2);
      memcpy_m128i(r1, r2);
      memcpy_m128i(r2, s[i].si);
    }
  }

#endif // #ifdef __SSE2__
};

typedef sfmt<607, 2, 15, 3, 13, 3,
             0xfdff37ffU, 0xef7f3f7dU, 0xff777b7dU, 0x7ff7fb2fU,
             0x00000001U, 0x00000000U, 0x00000000U, 0x5986f054U> sfmt607;
typedef sfmt<1279, 7, 14, 3, 5, 1,
             0xf7fefffdU, 0x7fefcfffU, 0xaff3ef3fU, 0xb5ffff7fU,
             0x00000001U, 0x00000000U, 0x00000000U, 0x20000000U> sfmt1279;
typedef sfmt<2281, 12, 19, 1, 5, 1,
             0xbff7ffbfU, 0xfdfffffeU, 0xf7ffef7fU, 0xf2f7cbbfU,
             0x00000001U, 0x00000000U, 0x00000000U, 0x41dfa600U> sfmt2281;
typedef sfmt<4253, 17, 20, 1, 7, 1,
             0x9f7bffffU, 0x9fffff5fU, 0x3efffffbU, 0xfffff7bbU,
             0xa8000001U, 0xaf5390a3U, 0xb740b3f8U, 0x6c11486dU> sfmt4253;
typedef sfmt<11213, 68, 14, 3, 7, 3,
             0xeffff7fbU, 0xffffffefU, 0xdfdfbfffU, 0x7fffdbfdU,
             0x00000001U, 0x00000000U, 0xe8148000U, 0xd0c7afa3U> sfmt11213;
typedef sfmt<19937, 122, 18, 1, 11, 1,
             0xdfffffefU, 0xddfecb7fU, 0xbffaffffU, 0xbffffff6U,
             0x00000001U, 0x00000000U, 0x00000000U, 0x13c9e684U> sfmt19937;
typedef sfmt<44497, 330, 5, 3, 9, 3,
             0xeffffffbU, 0xdfbebfffU, 0xbfbf7befU, 0x9ffd7bffU,
             0x00000001U, 0x00000000U, 0xa3ac4000U, 0xecc1327aU> sfmt44497;
typedef sfmt<86243, 366, 6, 7, 19, 1,
             0xfdbffbffU, 0xbff7ff3fU, 0xfd77efffU, 0xbf9ff3ffU,
             0x00000001U, 0x00000000U, 0x00000000U, 0xe9528d85U> sfmt86243;
typedef sfmt<132049, 110, 19, 1, 21, 1,
             0xffffbb5fU, 0xfb6ebf95U, 0xfffefffaU, 0xcff77fffU,
             0x00000001U, 0x00000000U, 0xcb520000U, 0xc7e91c7dU> sfmt132049;
typedef sfmt<216091, 627, 11, 3, 10, 1,
             0xbff7bff7U, 0xbfffffffU, 0xbffffa7fU, 0xffddfbfbU,
             0xf8000001U, 0x89e80709U, 0x3bd2b64bU, 0x0c64b1e4U> sfmt216091;

} // random
} // math
} // util
} // jubatus

#endif  // #ifndef JUBATUS_UTIL_MATH_RANDOM_SFMT_H_
