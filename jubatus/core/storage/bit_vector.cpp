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

#include "bit_vector.hpp"

#ifdef JUBATUS_USE_FMV
#include <nmmintrin.h>
#endif

namespace jubatus {
namespace core {
namespace storage {
namespace detail {
namespace {

#ifdef JUBATUS_USE_FMV
__attribute__((target("default")))
#endif
size_t calc_hamming_distance_impl(const uint64_t *x,
                                  const uint64_t *y,
                                  size_t blocks) {
  size_t match_num = 0;
  for (size_t i = 0; i < blocks; ++i) {
    match_num += bitcount(x[i] ^ y[i]);
  }
  return match_num;
}

#ifdef JUBATUS_USE_FMV
__attribute__((target("default")))
#endif
size_t bit_count_impl(const uint64_t *x, size_t blocks) {
  size_t result = 0;
  for (size_t i = 0; i < blocks; ++i) {
    result += bitcount(x[i]);
  }
  return result;
}

#ifdef JUBATUS_USE_FMV
__attribute__((target("popcnt")))
size_t calc_hamming_distance_impl(const uint64_t *x,
                                  const uint64_t *y,
                                  size_t blocks) {
  size_t match_num = 0;
#ifdef __x86_64__
  ssize_t i;
  for (i = 0; i < static_cast<ssize_t>(blocks) - 3; i += 4) {
    match_num += _mm_popcnt_u64(x[i] ^ y[i]);
    match_num += _mm_popcnt_u64(x[i + 1] ^ y[i + 1]);
    match_num += _mm_popcnt_u64(x[i + 2] ^ y[i + 2]);
    match_num += _mm_popcnt_u64(x[i + 3] ^ y[i + 3]);
  }
  for (; i < blocks; ++i) {
    match_num += _mm_popcnt_u64(x[i] ^ y[i]);
  }
#else  // #ifdef __x86_64__
  const uint32_t *p0 = (const uint32_t*)x;
  const uint32_t *p1 = (const uint32_t*)y;
  blocks *= 2;
  for (size_t i = 0; i < blocks; ++i) {
    match_num += _mm_popcnt_u32(p0[i] ^ p1[i]);
  }
#endif  // #ifdef __x86_64__
  return match_num;
}

__attribute__((target("popcnt")))
size_t bit_count_impl(const uint64_t *x, size_t blocks) {
  size_t result = 0;
#ifdef __x86_64__
  ssize_t i;
  for (i = 0; i < static_cast<ssize_t>(blocks) - 3; i += 4) {
    result += _mm_popcnt_u64(x[i]);
    result += _mm_popcnt_u64(x[i + 1]);
    result += _mm_popcnt_u64(x[i + 2]);
    result += _mm_popcnt_u64(x[i + 3]);
  }
  for (; i < blocks; ++i) {
    result += _mm_popcnt_u64(x[i]);
  }
#else  // #ifdef __x86_64__
  const uint32_t *p = (const uint32_t*)x;
  blocks *= 2;
  for (size_t i = 0; i < blocks; ++i) {
    result += _mm_popcnt_u32(p[i]);
  }
#endif  // #ifdef __x86_64__
  return result;
}
#endif

}  // namespace

size_t calc_hamming_distance_internal(
    const uint64_t *x, const uint64_t *y, size_t blocks) {
  return calc_hamming_distance_impl(x, y, blocks);
}

size_t bit_count_internal(const uint64_t *x, size_t blocks) {
  return bit_count_impl(x, blocks);
}

}  // namespace detail
}  // namespace storage
}  // namespace core
}  // namespace jubatus
