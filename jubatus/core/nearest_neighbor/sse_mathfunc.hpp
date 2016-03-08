/* SIMD (SSE1+MMX or SSE2) implementation of sin, cos, exp and log

   Inspired by Intel Approximate Math library, and based on the
   corresponding algorithms of the cephes math library

   The default is to use the SSE1 version. If you define USE_SSE2 the
   the SSE2 intrinsics will be used in place of the MMX intrinsics. Do
   not expect any significant performance improvement with SSE2.
*/
/* Copyright (C) 2007  Julien Pommier

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
   log_ps and sincos_ps function is based from below URL.
   http://gruntthepeon.free.fr/ssemath/

   Modifications:
   * remove SSE1+MMX code
   * remove other math functions
   * function multiversioning friendly
   * fixed that cpplint pointed out
*/

#ifndef JUBATUS_CORE_NEAREST_NEIGHBOR_SSE_MATHFUNC_HPP_
#define JUBATUS_CORE_NEAREST_NEIGHBOR_SSE_MATHFUNC_HPP_
#if defined(__SSE2__) || defined(JUBATUS_USE_FMV)

#if defined(JUBATUS_USE_FMV)
#include <immintrin.h>
#elif defined(__SSE2__)
#include <emmintrin.h>
#endif
#include "jubatus/util/math/constant.h"

#define _PS_CONST(Name, Val)                                            \
  static const float _ps_##Name##_f32[4] __attribute__((aligned(16))) = { \
    Val, Val, Val, Val};                                                \
  static const __m128 *_ps_##Name =                                     \
    reinterpret_cast<const __m128*>(_ps_##Name##_f32)
#define _PI_CONST(Name, Val)                                            \
  static const int _pi_##Name##_i32[4] __attribute__((aligned(16))) = { \
    Val, Val, Val, Val};                                                \
  static const __m128i *_pi_##Name =                                    \
    reinterpret_cast<const __m128i*>(_pi_##Name##_i32)

_PS_CONST(1  , 1.0f);
_PS_CONST(0p5, 0.5f);
_PI_CONST(min_norm_pos, 0x00800000);
_PI_CONST(inv_mant_mask, ~0x7f800000);
_PI_CONST(sign_mask, static_cast<int>(0x80000000));
_PI_CONST(inv_sign_mask, ~0x80000000);
_PI_CONST(1, 1);
_PI_CONST(inv1, ~1);
_PI_CONST(2, 2);
_PI_CONST(4, 4);
_PI_CONST(0x7f, 0x7f);
_PS_CONST(cephes_SQRTHF, 0.707106781186547524);
_PS_CONST(cephes_log_p0, 7.0376836292E-2);
_PS_CONST(cephes_log_p1, - 1.1514610310E-1);
_PS_CONST(cephes_log_p2, 1.1676998740E-1);
_PS_CONST(cephes_log_p3, - 1.2420140846E-1);
_PS_CONST(cephes_log_p4, + 1.4249322787E-1);
_PS_CONST(cephes_log_p5, - 1.6668057665E-1);
_PS_CONST(cephes_log_p6, + 2.0000714765E-1);
_PS_CONST(cephes_log_p7, - 2.4999993993E-1);
_PS_CONST(cephes_log_p8, + 3.3333331174E-1);
_PS_CONST(cephes_log_q1, -2.12194440e-4);
_PS_CONST(cephes_log_q2, 0.693359375);
_PS_CONST(minus_cephes_DP1, -0.78515625);
_PS_CONST(minus_cephes_DP2, -2.4187564849853515625e-4);
_PS_CONST(minus_cephes_DP3, -3.77489497744594108e-8);
_PS_CONST(sincof_p0, -1.9515295891E-4);
_PS_CONST(sincof_p1,  8.3321608736E-3);
_PS_CONST(sincof_p2, -1.6666654611E-1);
_PS_CONST(coscof_p0,  2.443315711809948E-005);
_PS_CONST(coscof_p1, -1.388731625493765E-003);
_PS_CONST(coscof_p2,  4.166664568298827E-002);
_PS_CONST(cephes_FOPI, 1.27323954473516);  // 4 / M_PI
_PS_CONST(minus2, -2.0f);
_PS_CONST(scale, static_cast<float>(1.0 / 16777216.0));
_PS_CONST(twopi, static_cast<float>(2.0 * jubatus::util::math::pi));

#ifdef JUBATUS_USE_FMV
__attribute__((target("sse2")))
#endif
__m128 log_ps(__m128 x) {
  __m128i emm0;
  __m128 one = *_ps_1;
  __m128 invalid_mask = _mm_cmple_ps(x, _mm_setzero_ps());
  x = _mm_max_ps(x, *reinterpret_cast<const __m128*>(_pi_min_norm_pos));
  emm0 = _mm_srli_epi32(_mm_castps_si128(x), 23);
  x = _mm_and_ps(x, *reinterpret_cast<const __m128*>(_pi_inv_mant_mask));
  x = _mm_or_ps(x, *_ps_0p5);
  emm0 = _mm_sub_epi32(emm0, *_pi_0x7f);
  __m128 e = _mm_cvtepi32_ps(emm0);
  e = _mm_add_ps(e, one);
  __m128 mask = _mm_cmplt_ps(x, *_ps_cephes_SQRTHF);
  __m128 tmp = _mm_and_ps(x, mask);
  x = _mm_sub_ps(x, one);
  e = _mm_sub_ps(e, _mm_and_ps(one, mask));
  x = _mm_add_ps(x, tmp);
  __m128 z = _mm_mul_ps(x, x);
  __m128 y = *_ps_cephes_log_p0;
  y = _mm_mul_ps(y, x);
  y = _mm_add_ps(y, *_ps_cephes_log_p1);
  y = _mm_mul_ps(y, x);
  y = _mm_add_ps(y, *_ps_cephes_log_p2);
  y = _mm_mul_ps(y, x);
  y = _mm_add_ps(y, *_ps_cephes_log_p3);
  y = _mm_mul_ps(y, x);
  y = _mm_add_ps(y, *_ps_cephes_log_p4);
  y = _mm_mul_ps(y, x);
  y = _mm_add_ps(y, *_ps_cephes_log_p5);
  y = _mm_mul_ps(y, x);
  y = _mm_add_ps(y, *_ps_cephes_log_p6);
  y = _mm_mul_ps(y, x);
  y = _mm_add_ps(y, *_ps_cephes_log_p7);
  y = _mm_mul_ps(y, x);
  y = _mm_add_ps(y, *_ps_cephes_log_p8);
  y = _mm_mul_ps(y, x);
  y = _mm_mul_ps(y, z);
  tmp = _mm_mul_ps(e, *_ps_cephes_log_q1);
  y = _mm_add_ps(y, tmp);
  tmp = _mm_mul_ps(z, *_ps_0p5);
  y = _mm_sub_ps(y, tmp);
  tmp = _mm_mul_ps(e, *_ps_cephes_log_q2);
  x = _mm_add_ps(x, y);
  x = _mm_add_ps(x, tmp);
  x = _mm_or_ps(x, invalid_mask);  // negative arg will be NAN
  return x;
}

#ifdef JUBATUS_USE_FMV
__attribute__((target("sse2")))
#endif
void sincos_ps(__m128 x, __m128 *s, __m128 *c) {
  __m128 xmm1, xmm2, xmm3 = _mm_setzero_ps(), sign_bit_sin, y;
  __m128i emm0, emm2, emm4;
  sign_bit_sin = x;
  x = _mm_and_ps(x, *reinterpret_cast<const __m128*>(_pi_inv_sign_mask));
  sign_bit_sin = _mm_and_ps(sign_bit_sin,
                            *reinterpret_cast<const __m128*>(_pi_sign_mask));
  y = _mm_mul_ps(x, *_ps_cephes_FOPI);
  emm2 = _mm_cvttps_epi32(y);
  emm2 = _mm_add_epi32(emm2, *_pi_1);
  emm2 = _mm_and_si128(emm2, *_pi_inv1);
  y = _mm_cvtepi32_ps(emm2);
  emm4 = emm2;
  emm0 = _mm_and_si128(emm2, *_pi_4);
  emm0 = _mm_slli_epi32(emm0, 29);
  __m128 swap_sign_bit_sin = _mm_castsi128_ps(emm0);
  emm2 = _mm_and_si128(emm2, *_pi_2);
  emm2 = _mm_cmpeq_epi32(emm2, _mm_setzero_si128());
  __m128 poly_mask = _mm_castsi128_ps(emm2);
  xmm1 = *_ps_minus_cephes_DP1;
  xmm2 = *_ps_minus_cephes_DP2;
  xmm3 = *_ps_minus_cephes_DP3;
  xmm1 = _mm_mul_ps(y, xmm1);
  xmm2 = _mm_mul_ps(y, xmm2);
  xmm3 = _mm_mul_ps(y, xmm3);
  x = _mm_add_ps(x, xmm1);
  x = _mm_add_ps(x, xmm2);
  x = _mm_add_ps(x, xmm3);
  emm4 = _mm_sub_epi32(emm4, *_pi_2);
  emm4 = _mm_andnot_si128(emm4, *_pi_4);
  emm4 = _mm_slli_epi32(emm4, 29);
  __m128 sign_bit_cos = _mm_castsi128_ps(emm4);
  sign_bit_sin = _mm_xor_ps(sign_bit_sin, swap_sign_bit_sin);
  __m128 z = _mm_mul_ps(x, x);
  y = *_ps_coscof_p0;
  y = _mm_mul_ps(y, z);
  y = _mm_add_ps(y, *_ps_coscof_p1);
  y = _mm_mul_ps(y, z);
  y = _mm_add_ps(y, *_ps_coscof_p2);
  y = _mm_mul_ps(y, z);
  y = _mm_mul_ps(y, z);
  __m128 tmp = _mm_mul_ps(z, *_ps_0p5);
  y = _mm_sub_ps(y, tmp);
  y = _mm_add_ps(y, *_ps_1);
  __m128 y2 = *_ps_sincof_p0;
  y2 = _mm_mul_ps(y2, z);
  y2 = _mm_add_ps(y2, *_ps_sincof_p1);
  y2 = _mm_mul_ps(y2, z);
  y2 = _mm_add_ps(y2, *_ps_sincof_p2);
  y2 = _mm_mul_ps(y2, z);
  y2 = _mm_mul_ps(y2, x);
  y2 = _mm_add_ps(y2, x);
  xmm3 = poly_mask;
  __m128 ysin2 = _mm_and_ps(xmm3, y2);
  __m128 ysin1 = _mm_andnot_ps(xmm3, y);
  y2 = _mm_sub_ps(y2, ysin2);
  y = _mm_sub_ps(y, ysin1);
  xmm1 = _mm_add_ps(ysin1, ysin2);
  xmm2 = _mm_add_ps(y, y2);
  *s = _mm_xor_ps(xmm1, sign_bit_sin);
  *c = _mm_xor_ps(xmm2, sign_bit_cos);
}

#endif  // #if defined(__SSE2__) || defined(JUBATUS_USE_FMV)
#endif  // #ifndef JUBATUS_CORE_NEAREST_NEIGHBOR_SSE_MATHFUNC_HPP_
