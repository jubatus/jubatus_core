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

#include <algorithm>
#include <vector>
#include "../common/hash.hpp"
#include "../common/thread_pool.hpp"
#include "jubatus/util/math/random.h"
#include "sse_mathfunc.hpp"
#include "avx_mathfunc.hpp"
#include "bit_vector_ranking.hpp"

using std::vector;
using jubatus::core::storage::bit_vector;
using jubatus::core::nearest_neighbor::cache_t;
using jubatus::util::concurrent::scoped_lock;

namespace jubatus {
namespace core {
namespace nearest_neighbor {
namespace {

std::vector<float> random_projection_internal(
    const common::sfv_t& sfv,
    uint32_t hash_num,
    size_t start,
    size_t end,
    cache_t& cache);

#if defined(__SSE2__) || defined(JUBATUS_USE_FMV)
template <class RND>
inline void next_gaussian_float8(RND& g, float* out);
#endif

#ifdef JUBATUS_USE_FMV
__attribute__((target("default")))
std::vector<float> random_projection_internal(
    const common::sfv_t& sfv,
    uint32_t hash_num,
    size_t start,
    size_t end,
    cache_t& cache);

__attribute__((target("sse2")))
std::vector<float> random_projection_internal(
    const common::sfv_t& sfv,
    uint32_t hash_num,
    size_t start,
    size_t end,
    cache_t& cache);

__attribute__((target("avx2")))
std::vector<float> random_projection_internal(
    const common::sfv_t& sfv,
    uint32_t hash_num,
    size_t start,
    size_t end,
    cache_t& cache);

template <class RND> __attribute__((target("avx2")))
inline void next_gaussian_float16(RND& g, float* out);
#endif

std::vector<float> random_projection_dispatcher(
    const common::sfv_t* sfv,
    uint32_t hash_num,
    size_t start,
    size_t end,
    cache_t* cache) {
  return random_projection_internal(*sfv, hash_num, start, end, *cache);
}

inline static void init_cache(
    uint32_t hash_num,
    vector<float>& proj,
    const cache_t& cache);
inline static bool check_cache(
    cache_t& cache,
    uint32_t seed,
    vector<float>& proj,
    float v);
template<int N>
inline static void build_cache_if_enabled(
    const cache_t& cache,
    vector<float>& grnd_cache,
    const float *grnd);
inline static void lru_set_cache_if_enabled(
    cache_t& cache,
    uint32_t seed,
    vector<float>& grnd_cache);

}  // namespace

vector<float> random_projection(
    const common::sfv_t& sfv,
    uint32_t hash_num,
    uint32_t threads,
    cache_t& cache) {
  typedef std::vector<
    jubatus::util::lang::shared_ptr<
      common::thread_pool::future<std::vector<float> > > > future_list_t;
  if (threads > 1 && sfv.size() > 0) {
    size_t block_size =
      static_cast<size_t>(std::ceil(sfv.size() / static_cast<float>(threads)));
    vector<float> proj(hash_num);
    std::vector<jubatus::util::lang::function<std::vector<float>()> > funcs;
    funcs.reserve(sfv.size() / block_size + 1);
    for (size_t t = 0, end = 0; t < threads && end < sfv.size() ; ++t) {
      size_t off = end;
      end += std::min(block_size, sfv.size() - off);
      funcs.push_back(jubatus::util::lang::bind(
        &random_projection_dispatcher, &sfv, hash_num, off, end, &cache));
    }
    future_list_t futures =
      jubatus::core::common::default_thread_pool::async_all(funcs);
    for (future_list_t::iterator it = futures.begin();
         it != futures.end(); ++it) {
      const std::vector<float>& pj = (*it)->get();
      for (size_t i = 0; i < proj.size(); ++i)
        proj[i] += pj[i];
    }
    return proj;
  } else {
    return random_projection_internal(sfv, hash_num, 0, sfv.size(), cache);
  }
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

bit_vector cosine_lsh(
    const common::sfv_t& sfv,
    uint32_t hash_num,
    uint32_t threads,
    cache_t& cache) {
  return binarize(random_projection(sfv, hash_num, threads, cache));
}

namespace {

#if !defined(__SSE2__) || defined(JUBATUS_USE_FMV)
#ifdef JUBATUS_USE_FMV
__attribute__((target("default")))
#endif
vector<float> random_projection_internal(
    const common::sfv_t& sfv,
    uint32_t hash_num,
    size_t start,
    size_t end,
    cache_t& cache) {
  vector<float> proj(hash_num);
  std::vector<float> grnd_cache;
  init_cache(hash_num, grnd_cache, cache);
  for (size_t i = start; i < end; ++i) {
    const uint32_t seed = common::hash_util::calc_string_hash(sfv[i].first);
    const float v = sfv[i].second;
    if (check_cache(cache, seed, proj, v)) {
      continue;
    }
    jubatus::util::math::random::sfmt607rand rnd(seed);
    for (uint32_t j = 0; j < hash_num; ++j) {
      const float r = rnd.next_gaussian_float();
      proj[j] += v * r;
      build_cache_if_enabled<1>(cache, grnd_cache, &r);
    }
    lru_set_cache_if_enabled(cache, seed, grnd_cache);
  }
  return proj;
}
#endif  // #if !defined(__SSE2__) || defined(JUBATUS_USE_FMV)

#if defined(__SSE2__) || defined(JUBATUS_USE_FMV)
#ifdef JUBATUS_USE_FMV
__attribute__((target("sse2")))
#endif
vector<float> random_projection_internal(
    const common::sfv_t& sfv,
    uint32_t hash_num,
    size_t start,
    size_t end,
    cache_t& cache) {
  std::vector<float> proj(hash_num);
  float *p = const_cast<float*>(proj.data());
  uint32_t hash_num_sse = hash_num & 0xfffffff8;
  float grnd[8] __attribute__((aligned(16)));
  std::vector<float> grnd_cache;
  init_cache(hash_num, grnd_cache, cache);
  for (size_t i = start; i < end; ++i) {
    const uint32_t seed = common::hash_util::calc_string_hash(sfv[i].first);
    const float v = sfv[i].second;
    if (check_cache(cache, seed, proj, v)) {
      continue;
    }
    jubatus::util::math::random::sfmt607rand rnd(seed);
    __m128 v4 = _mm_set1_ps(v);
    uint32_t j = 0;
    for (; j < hash_num_sse; j += 8) {
      next_gaussian_float8(rnd, grnd);
      build_cache_if_enabled<8>(cache, grnd_cache, grnd);
      __m128 t0 = _mm_loadu_ps(p + j);
      __m128 t1 = _mm_loadu_ps(p + j + 4);
      __m128 t2 = _mm_mul_ps(v4, _mm_load_ps(grnd));
      __m128 t3 = _mm_mul_ps(v4, _mm_load_ps(grnd + 4));
      _mm_storeu_ps(p + j + 0, _mm_add_ps(t0, t2));
      _mm_storeu_ps(p + j + 4, _mm_add_ps(t1, t3));
    }
    for (; j < hash_num; ++j) {
      const float r = rnd.next_gaussian_float();
      proj[j] += v * r;
      build_cache_if_enabled<1>(cache, grnd_cache, &r);
    }
    lru_set_cache_if_enabled(cache, seed, grnd_cache);
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
    g.fill_int_unsafe(reinterpret_cast<uint32_t*>(&(t[0])), 8);
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

#endif  // #if defined(__SSE2__) || defined(JUBATUS_USE_FMV)

#ifdef JUBATUS_USE_FMV
__attribute__((target("avx2")))
vector<float> random_projection_internal(
    const common::sfv_t& sfv,
    uint32_t hash_num,
    size_t start,
    size_t end,
    cache_t& cache) {
  std::vector<float> proj(hash_num);
  float *p = const_cast<float*>(proj.data());
  uint32_t hash_num_avx = hash_num & 0xfffffff0;
  float grnd[16] __attribute__((aligned(32)));
  std::vector<float> grnd_cache;
  init_cache(hash_num, grnd_cache, cache);
  for (size_t i = start; i < end; ++i) {
    const uint32_t seed = common::hash_util::calc_string_hash(sfv[i].first);
    const float v = sfv[i].second;
    if (check_cache(cache, seed, proj, v)) {
      continue;
    }
    jubatus::util::math::random::sfmt607rand rnd(seed);
    __m256 v8 = _mm256_set1_ps(v);
    uint32_t j = 0;
    for (; j < hash_num_avx; j += 16) {
      next_gaussian_float16(rnd, grnd);
      build_cache_if_enabled<16>(cache, grnd_cache, grnd);
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
        build_cache_if_enabled<8>(cache, grnd_cache, grnd);
        __m128 t0 = _mm_loadu_ps(p + j);
        __m128 t1 = _mm_loadu_ps(p + j + 4);
        __m128 t2 = _mm_mul_ps(v4, _mm_load_ps(grnd));
        __m128 t3 = _mm_mul_ps(v4, _mm_load_ps(grnd + 4));
        _mm_storeu_ps(p + j + 0, _mm_add_ps(t0, t2));
        _mm_storeu_ps(p + j + 4, _mm_add_ps(t1, t3));
      }
      for (; j < hash_num; ++j) {
        const float r = rnd.next_gaussian_float();
        proj[j] += v * r;
        build_cache_if_enabled<1>(cache, grnd_cache, &r);
      }
    }
    lru_set_cache_if_enabled(cache, seed, grnd_cache);
  }
  return proj;
}

template <class RND> __attribute__((target("avx2")))
inline static void next_gaussian_float16(RND& g, float *out) {
  __m256 a, b;
  {
    __m256i t[2] __attribute__((aligned(32)));
    g.fill_int_unsafe(reinterpret_cast<uint32_t*>(&(t[0])), 16);
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

inline static void init_cache(
    uint32_t hash_num,
    vector<float>& proj,
    const cache_t& cache) {
  if (cache.bool_test()) {
    proj.reserve(hash_num);
  }
}

inline static bool check_cache(
    cache_t& cache,
    uint32_t seed,
    vector<float>& proj,
    float v) {
  if (cache.bool_test()) {
    scoped_lock lk(cache->lock);
    if (cache->lru.has(seed)) {
      const std::vector<float>& lst = cache->lru.get(seed);
      for (size_t j = 0; j < lst.size(); ++j) {
        proj[j] += v * lst[j];
      }
      return true;
    }
  }
  return false;
}

template<int N>
inline static void build_cache_if_enabled(
    const cache_t& cache,
    vector<float>& grnd_cache,
    const float *grnd) {
  if (cache.bool_test()) {
    for (int i = 0; i < N; ++i)
      grnd_cache.push_back(grnd[i]);
  }
}

inline static void lru_set_cache_if_enabled(
    cache_t& cache,
    uint32_t seed,
    vector<float>& grnd_cache) {
  if (cache.bool_test()) {
    scoped_lock lk(cache->lock);
    if (cache->lru.has(seed)) {
      cache->lru.touch(seed);
    } else {
      cache->lru.set(seed, grnd_cache);
    }
    grnd_cache.clear();
  }
}

}  // namespace
}  // namespace nearest_neighbor
}  // namespace core
}  // namespace jubatus
