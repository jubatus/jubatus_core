// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2011 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#include "lsh_function.hpp"

#include <vector>
#include "../common/hash.hpp"
#include "jubatus/util/math/random.h"
#include "sse_mathfunc.hpp"
#include "avx_mathfunc.hpp"

using std::vector;
using jubatus::core::storage::bit_vector;

namespace jubatus {
namespace core {
namespace nearest_neighbor {
namespace {

std::vector<float> random_projection_internal(
  const common::sfv_t& sfv, uint32_t hash_num);

#if defined(__SSE2__) || defined(JUBATUS_USE_FMV)
template <class RND>
inline void next_gaussian_float8(RND& g, float *out);
#endif

#ifdef JUBATUS_USE_FMV
__attribute__((target("default")))
std::vector<float> random_projection_internal(
  const common::sfv_t& sfv, uint32_t hash_num);

__attribute__((target("sse2")))
std::vector<float> random_projection_internal(
  const common::sfv_t& sfv, uint32_t hash_num);

__attribute__((target("avx2")))
std::vector<float> random_projection_internal(
  const common::sfv_t& sfv, uint32_t hash_num);

template <class RND> __attribute__((target("avx2")))
inline void next_gaussian_float16(RND& g, float *out);
#endif
}

vector<float> random_projection(const common::sfv_t& sfv, uint32_t hash_num) {
  return random_projection_internal(sfv, hash_num);
}

bit_vector binarize(const vector<float>& proj) {
  bit_vector bv(proj.size());
  for (size_t i = 0; i < proj.size(); ++i) {
    if (proj[i] > 0) {
      bv.set_bit(i);
    }
  }
  return bv;
}

bit_vector cosine_lsh(const common::sfv_t& sfv, uint32_t hash_num) {
  return binarize(random_projection(sfv, hash_num));
}

namespace {

#if !defined(__SSE2__) || defined(JUBATUS_USE_FMV)
#ifdef JUBATUS_USE_FMV
__attribute__((target("default")))
#endif
vector<float> random_projection_internal(const common::sfv_t& sfv,
                                         uint32_t hash_num) {
  vector<float> proj(hash_num);
  for (size_t i = 0; i < sfv.size(); ++i) {
    const uint32_t seed = common::hash_util::calc_string_hash(sfv[i].first);
    jubatus::util::math::random::mtrand rnd(seed);
    for (uint32_t j = 0; j < hash_num; ++j) {
      proj[j] += sfv[i].second * rnd.next_gaussian_float();
    }
  }
  return proj;
}
#endif  // #if !defined(__SSE2__) || defined(JUBATUS_ENABLED_FUNCTION_MULTIV...

#if defined(__SSE2__) || defined(JUBATUS_USE_FMV)
#ifdef JUBATUS_USE_FMV
__attribute__((target("sse2")))
#endif
vector<float> random_projection_internal(const common::sfv_t& sfv,
                                         uint32_t hash_num) {
  std::vector<float> proj(hash_num);
  float *p = const_cast<float*>(proj.data());
  uint32_t hash_num_sse = hash_num & 0xfffffff8;
  float grnd[8] __attribute__((aligned(16)));
  for (size_t i = 0; i < sfv.size(); ++i) {
    const uint32_t seed = common::hash_util::calc_string_hash(sfv[i].first);
    jubatus::util::math::random::mtrand rnd(seed);
    const float v = sfv[i].second;
    __m128 v4 = _mm_set1_ps(v);
    uint32_t j = 0;
    for (; j < hash_num_sse; j += 8) {
      next_gaussian_float8(rnd, grnd);
      __m128 t0 = _mm_loadu_ps(p + j);
      __m128 t1 = _mm_loadu_ps(p + j + 4);
      __m128 t2 = _mm_mul_ps(v4, _mm_load_ps(grnd));
      __m128 t3 = _mm_mul_ps(v4, _mm_load_ps(grnd + 4));
      _mm_storeu_ps(p + j + 0, _mm_add_ps(t0, t2));
      _mm_storeu_ps(p + j + 4, _mm_add_ps(t1, t3));
    }
    for (; j < hash_num; ++j) {
      proj[j] += v * rnd.next_gaussian_float();
    }
  }
  return proj;
}

template <class RND>
#ifdef JUBATUS_USE_FMV
__attribute__((target("sse2")))
#endif
inline void next_gaussian_float8(RND& g, float *out) {
  __m128 a, b;
  {
    __m128i t[2] __attribute__((aligned(16)));
    uint32_t *p = reinterpret_cast<uint32_t*>(&(t[0]));
    for (int i = 0; i < 8; ++i)
      p[i] = g.next_int();
    a = _mm_cvtepi32_ps(_mm_srli_epi32(t[0], 8));
    b = _mm_cvtepi32_ps(_mm_srli_epi32(t[1], 8));
    __m128 c = _mm_unpacklo_ps(a, b);
    __m128 d = _mm_unpackhi_ps(a, b);
    a = _mm_unpacklo_ps(c, d);
    b = _mm_unpackhi_ps(c, d);
  }
  a = _mm_mul_ps(a, *_ps_scale);
  b = _mm_mul_ps(b, *_ps_scale);
  a = _mm_sub_ps(*_ps_1, a);
  b = _mm_sub_ps(*_ps_1, b);
  b = _mm_mul_ps(b, *_ps_twopi);
  a = log_ps(a);
  __m128 s, c;
  sincos_ps(b, &s, &c);
  a = _mm_mul_ps(a, *_ps_minus2);
  a = _mm_sqrt_ps(a);
  b = _mm_mul_ps(a, s);
  a = _mm_mul_ps(a, c);
  _mm_storeu_ps(out + 0, _mm_unpacklo_ps(b, a));
  _mm_storeu_ps(out + 4, _mm_unpackhi_ps(b, a));
}

#endif  // #if defined(__SSE2__) || defined(JUBATUS_ENABLED_FUNCTION_MULTIVE...

#ifdef JUBATUS_USE_FMV
__attribute__((target("avx2")))
vector<float> random_projection_internal(const common::sfv_t& sfv,
                                         uint32_t hash_num) {
  std::vector<float> proj(hash_num);
  float *p = const_cast<float*>(proj.data());
  uint32_t hash_num_avx = hash_num & 0xfffffff0;
  float grnd[16] __attribute__((aligned(32)));
  for (size_t i = 0; i < sfv.size(); ++i) {
    const uint32_t seed = common::hash_util::calc_string_hash(sfv[i].first);
    jubatus::util::math::random::mtrand rnd(seed);
    const float v = sfv[i].second;
    __m256 v8 = _mm256_set1_ps(v);
    uint32_t j = 0;
    for (; j < hash_num_avx; j += 16) {
      next_gaussian_float16(rnd, grnd);
      __m256 t0 = _mm256_loadu_ps(p + j);
      __m256 t1 = _mm256_loadu_ps(p + j + 8);
      __m256 t2 = _mm256_mul_ps(v8, _mm256_load_ps(grnd));
      __m256 t3 = _mm256_mul_ps(v8, _mm256_load_ps(grnd + 8));
      _mm256_storeu_ps(p + j + 0, _mm256_add_ps(t0, t2));
      _mm256_storeu_ps(p + j + 8, _mm256_add_ps(t1, t3));
    }
    if (j < hash_num) {
      __m128 v4 = _mm_set1_ps(v);
      for (; j < hash_num - 7; j += 8) {
        next_gaussian_float8(rnd, grnd);
        __m128 t0 = _mm_loadu_ps(p + j);
        __m128 t1 = _mm_loadu_ps(p + j + 4);
        __m128 t2 = _mm_mul_ps(v4, _mm_load_ps(grnd));
        __m128 t3 = _mm_mul_ps(v4, _mm_load_ps(grnd + 4));
        _mm_storeu_ps(p + j + 0, _mm_add_ps(t0, t2));
        _mm_storeu_ps(p + j + 4, _mm_add_ps(t1, t3));
      }
      for (; j < hash_num; ++j) {
        proj[j] += v * rnd.next_gaussian_float();
      }
    }
  }
  return proj;
}

template <class RND> __attribute__((target("avx2")))
inline static void next_gaussian_float16(RND& g, float *out) {
  __m256 a, b;
  {
    __m256i t[2] __attribute__((aligned(32)));
    uint32_t *p = reinterpret_cast<uint32_t*>(&(t[0]));
    for (int i = 0; i < 16; ++i)
      p[i] = g.next_int();
    a = _mm256_cvtepi32_ps(_mm256_srli_epi32(t[0], 8));
    b = _mm256_cvtepi32_ps(_mm256_srli_epi32(t[1], 8));
    __m256 c = _mm256_unpacklo_ps(a, b);
    __m256 d = _mm256_unpackhi_ps(a, b);
    a = _mm256_unpacklo_ps(c, d);
    b = _mm256_unpackhi_ps(c, d);
  }
  a = _mm256_mul_ps(a, *_ps256_scale);
  b = _mm256_mul_ps(b, *_ps256_scale);
  a = _mm256_sub_ps(*_ps256_1, a);
  b = _mm256_sub_ps(*_ps256_1, b);
  b = _mm256_mul_ps(b, *_ps256_twopi);
  a = log256_ps(a);
  __m256 s, c;
  sincos256_ps(b, &s, &c);
  a = _mm256_mul_ps(a, *_ps256_minus2);
  a = _mm256_sqrt_ps(a);
  b = _mm256_mul_ps(a, s);
  a = _mm256_mul_ps(a, c);
  _mm256_store_ps(out + 0, _mm256_unpacklo_ps(b, a));
  _mm256_store_ps(out + 8, _mm256_unpackhi_ps(b, a));
}

#endif  // #ifdef JUBATUS_USE_FMV

}  // namespace
}  // namespace nearest_neighbor
}  // namespace core
}  // namespace jubatus
