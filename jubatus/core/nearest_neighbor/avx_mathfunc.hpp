/*
   AVX implementation of sin, cos, sincos, exp and log

   Based on "sse_mathfun.h", by Julien Pommier
   http://gruntthepeon.free.fr/ssemath/

   Copyright (C) 2012 Giovanni Garberoglio
   Interdisciplinary Laboratory for Computational Science (LISC)
   Fondazione Bruno Kessler and University of Trento
   via Sommarive, 18
   I-38123 Trento (Italy)

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  (this is the zlib license)
*/
/*
   log256_ps and sincos256_ps function is based from below URL.
   http://software-lisc.fbk.eu/avx_mathfun/

   Modifications:
   * remove other math functions
   * remove SSE fallback code
   * function multiversioning friendly
   * fixed compiler warning
   * fixed that cpplint pointed out
*/

#ifndef JUBATUS_CORE_NEAREST_NEIGHBOR_AVX_MATHFUNC_HPP_
#define JUBATUS_CORE_NEAREST_NEIGHBOR_AVX_MATHFUNC_HPP_

#ifdef JUBATUS_USE_FMV

#include <immintrin.h>
#include "jubatus/util/math/constant.h"

#define _PS256_CONST(Name, Val)                                         \
  static const float _ps256_##Name##_f32[8] __attribute__((aligned(32))) = { \
    Val, Val, Val, Val, Val, Val, Val, Val};                            \
  static const __m256 *_ps256_##Name =                                  \
    reinterpret_cast<const __m256*>(_ps256_##Name##_f32)
#define _PI256_CONST(Name, Val)                                         \
  static const int _pi256_##Name##_i32[8] __attribute__((aligned(32))) = { \
    Val, Val, Val, Val, Val, Val, Val, Val};                            \
  static const __m256i *_pi256_##Name =                                 \
    reinterpret_cast<const __m256i*>(_pi256_##Name##_i32)

_PI256_CONST(min_norm_pos, static_cast<int>(0x00800000));
_PI256_CONST(inv_mant_mask, static_cast<int>(~0x7f800000));
_PS256_CONST(cephes_log_p0, 7.0376836292E-2);
_PS256_CONST(cephes_log_p1, - 1.1514610310E-1);
_PS256_CONST(cephes_log_p2, 1.1676998740E-1);
_PS256_CONST(cephes_log_p3, - 1.2420140846E-1);
_PS256_CONST(cephes_log_p4, + 1.4249322787E-1);
_PS256_CONST(cephes_log_p5, - 1.6668057665E-1);
_PS256_CONST(cephes_log_p6, + 2.0000714765E-1);
_PS256_CONST(cephes_log_p7, - 2.4999993993E-1);
_PS256_CONST(cephes_log_p8, + 3.3333331174E-1);
_PS256_CONST(cephes_log_q1, -2.12194440e-4);
_PS256_CONST(cephes_log_q2, 0.693359375);
_PS256_CONST(cephes_SQRTHF, 0.707106781186547524);
_PI256_CONST(0x7f, 0x7f);
_PI256_CONST(sign_mask, static_cast<int>(0x80000000));
_PI256_CONST(inv_sign_mask, static_cast<int>(~0x80000000));
_PS256_CONST(cephes_FOPI, 1.27323954473516);  // 4 / M_PI
_PS256_CONST(minus_cephes_DP1, -0.78515625);
_PS256_CONST(minus_cephes_DP2, -2.4187564849853515625e-4);
_PS256_CONST(minus_cephes_DP3, -3.77489497744594108e-8);
_PS256_CONST(sincof_p0, -1.9515295891E-4);
_PS256_CONST(sincof_p1,  8.3321608736E-3);
_PS256_CONST(sincof_p2, -1.6666654611E-1);
_PS256_CONST(coscof_p0,  2.443315711809948E-005);
_PS256_CONST(coscof_p1, -1.388731625493765E-003);
_PS256_CONST(coscof_p2,  4.166664568298827E-002);
_PS256_CONST(1  , 1.0f);
_PS256_CONST(0p5, 0.5f);
_PI256_CONST(0, 0);
_PI256_CONST(1, 1);
_PI256_CONST(inv1, ~1);
_PI256_CONST(2, 2);
_PI256_CONST(4, 4);
_PS256_CONST(minus2, -2.0f);
_PS256_CONST(scale , static_cast<float>(1.0 / 16777216.0));
_PS256_CONST(twopi , static_cast<float>(2.0 * jubatus::util::math::pi));

__attribute__((target("avx2")))
static inline __m256 log256_ps(__m256 x) {
  __m256i imm0;
  __m256 one = *_ps256_1;
  __m256 invalid_mask = _mm256_cmp_ps(x, _mm256_setzero_ps(), _CMP_LE_OS);
  x = _mm256_max_ps(x, *reinterpret_cast<const __m256*>(_pi256_min_norm_pos));
  imm0 = _mm256_srli_epi32(_mm256_castps_si256(x), 23);
  x = _mm256_and_ps(x, *reinterpret_cast<const __m256*>(_pi256_inv_mant_mask));
  x = _mm256_or_ps(x, *_ps256_0p5);
  imm0 = _mm256_sub_epi32(imm0, *_pi256_0x7f);
  __m256 e = _mm256_cvtepi32_ps(imm0);
  e = _mm256_add_ps(e, one);
  __m256 mask = _mm256_cmp_ps(x, *_ps256_cephes_SQRTHF, _CMP_LT_OS);
  __m256 tmp = _mm256_and_ps(x, mask);
  x = _mm256_sub_ps(x, one);
  e = _mm256_sub_ps(e, _mm256_and_ps(one, mask));
  x = _mm256_add_ps(x, tmp);
  __m256 z = _mm256_mul_ps(x, x);
  __m256 y = *_ps256_cephes_log_p0;
  y = _mm256_mul_ps(y, x);
  y = _mm256_add_ps(y, *_ps256_cephes_log_p1);
  y = _mm256_mul_ps(y, x);
  y = _mm256_add_ps(y, *_ps256_cephes_log_p2);
  y = _mm256_mul_ps(y, x);
  y = _mm256_add_ps(y, *_ps256_cephes_log_p3);
  y = _mm256_mul_ps(y, x);
  y = _mm256_add_ps(y, *_ps256_cephes_log_p4);
  y = _mm256_mul_ps(y, x);
  y = _mm256_add_ps(y, *_ps256_cephes_log_p5);
  y = _mm256_mul_ps(y, x);
  y = _mm256_add_ps(y, *_ps256_cephes_log_p6);
  y = _mm256_mul_ps(y, x);
  y = _mm256_add_ps(y, *_ps256_cephes_log_p7);
  y = _mm256_mul_ps(y, x);
  y = _mm256_add_ps(y, *_ps256_cephes_log_p8);
  y = _mm256_mul_ps(y, x);
  y = _mm256_mul_ps(y, z);
  tmp = _mm256_mul_ps(e, *_ps256_cephes_log_q1);
  y = _mm256_add_ps(y, tmp);
  tmp = _mm256_mul_ps(z, *_ps256_0p5);
  y = _mm256_sub_ps(y, tmp);
  tmp = _mm256_mul_ps(e, *_ps256_cephes_log_q2);
  x = _mm256_add_ps(x, y);
  x = _mm256_add_ps(x, tmp);
  x = _mm256_or_ps(x, invalid_mask);
  return x;
}

__attribute__((target("avx2")))
static inline void sincos256_ps(__m256 x, __m256 *s, __m256 *c) {
  __m256 xmm1, xmm2, xmm3 = _mm256_setzero_ps(), sign_bit_sin, y;
  __m256i imm0, imm2, imm4;
  sign_bit_sin = x;
  x = _mm256_and_ps(x, *reinterpret_cast<const __m256*>(_pi256_inv_sign_mask));
  sign_bit_sin = _mm256_and_ps(
    sign_bit_sin, *reinterpret_cast<const __m256*>(_pi256_sign_mask));
  y = _mm256_mul_ps(x, *_ps256_cephes_FOPI);
  imm2 = _mm256_cvttps_epi32(y);
  imm2 = _mm256_add_epi32(imm2, *_pi256_1);
  imm2 = _mm256_and_si256(imm2, *_pi256_inv1);
  y = _mm256_cvtepi32_ps(imm2);
  imm4 = imm2;
  imm0 = _mm256_and_si256(imm2, *_pi256_4);
  imm0 = _mm256_slli_epi32(imm0, 29);
  imm2 = _mm256_and_si256(imm2, *_pi256_2);
  imm2 = _mm256_cmpeq_epi32(imm2, *_pi256_0);
  __m256 swap_sign_bit_sin = _mm256_castsi256_ps(imm0);
  __m256 poly_mask = _mm256_castsi256_ps(imm2);
  xmm1 = *_ps256_minus_cephes_DP1;
  xmm2 = *_ps256_minus_cephes_DP2;
  xmm3 = *_ps256_minus_cephes_DP3;
  xmm1 = _mm256_mul_ps(y, xmm1);
  xmm2 = _mm256_mul_ps(y, xmm2);
  xmm3 = _mm256_mul_ps(y, xmm3);
  x = _mm256_add_ps(x, xmm1);
  x = _mm256_add_ps(x, xmm2);
  x = _mm256_add_ps(x, xmm3);
  imm4 = _mm256_sub_epi32(imm4, *_pi256_2);
  imm4 = _mm256_andnot_si256(imm4, *_pi256_4);
  imm4 = _mm256_slli_epi32(imm4, 29);
  __m256 sign_bit_cos = _mm256_castsi256_ps(imm4);
  sign_bit_sin = _mm256_xor_ps(sign_bit_sin, swap_sign_bit_sin);
  __m256 z = _mm256_mul_ps(x, x);
  y = *_ps256_coscof_p0;
  y = _mm256_mul_ps(y, z);
  y = _mm256_add_ps(y, *_ps256_coscof_p1);
  y = _mm256_mul_ps(y, z);
  y = _mm256_add_ps(y, *_ps256_coscof_p2);
  y = _mm256_mul_ps(y, z);
  y = _mm256_mul_ps(y, z);
  __m256 tmp = _mm256_mul_ps(z, *_ps256_0p5);
  y = _mm256_sub_ps(y, tmp);
  y = _mm256_add_ps(y, *_ps256_1);
  __m256 y2 = *_ps256_sincof_p0;
  y2 = _mm256_mul_ps(y2, z);
  y2 = _mm256_add_ps(y2, *_ps256_sincof_p1);
  y2 = _mm256_mul_ps(y2, z);
  y2 = _mm256_add_ps(y2, *_ps256_sincof_p2);
  y2 = _mm256_mul_ps(y2, z);
  y2 = _mm256_mul_ps(y2, x);
  y2 = _mm256_add_ps(y2, x);
  xmm3 = poly_mask;
  __m256 ysin2 = _mm256_and_ps(xmm3, y2);
  __m256 ysin1 = _mm256_andnot_ps(xmm3, y);
  y2 = _mm256_sub_ps(y2, ysin2);
  y = _mm256_sub_ps(y, ysin1);
  xmm1 = _mm256_add_ps(ysin1, ysin2);
  xmm2 = _mm256_add_ps(y, y2);
  *s = _mm256_xor_ps(xmm1, sign_bit_sin);
  *c = _mm256_xor_ps(xmm2, sign_bit_cos);
}

#endif  // #ifdef  JUBATUS_USE_FMV

#endif  // JUBATUS_CORE_NEAREST_NEIGHBOR_AVX_MATHFUNC_HPP_
