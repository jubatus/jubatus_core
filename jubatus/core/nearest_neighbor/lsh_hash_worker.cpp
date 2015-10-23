// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2015 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#include <functional>
#include <utility>
#include <vector>
#include <queue>
#include "lsh_hash_worker.hpp"
#include "lsh_function.hpp"
#include "../common/hash.hpp"
#include "../common/type.hpp"
#include "../storage/bit_vector.hpp"
#include "jubatus/util/math/random.h"
#include "jubatus/util/lang/bind.h"
#include "jubatus/util/lang/function.h"
#include "jubatus/util/lang/shared_ptr.h"
#include "jubatus/util/data/unordered_map.h"

using std::make_pair;
using std::pair;
using std::vector;
using jubatus::core::common::sfv_t;
using jubatus::core::storage::bit_vector;
using jubatus::core::storage::const_bit_vector_column;

namespace jubatus {
namespace core {
namespace nearest_neighbor {
namespace detail {
inline void yield() {
#ifdef __linux__
  sched_yield();
#else
  (void)sleep(0);
#endif
}
}  // namespace detail

struct hash_task {
  const sfv_t& sfv;
  const size_t fv_offset;
  const size_t fv_length;
  uint32_t hash_num;
  volatile bool finished;
  bool is_finished() const {
    return finished;
  }
  vector<float> result;
  util::lang::function<void(util::lang::shared_ptr<hash_task>)> task;
  hash_task(const sfv_t& s,
            size_t o,
            size_t l,
            uint32_t h)
    : sfv(s),
      fv_offset(o),
      fv_length(l),
      hash_num(h),
      finished(false),
      result(h),
      task(hash_work)
  {}
};

typedef util::data::unordered_map<std::string, vector<float> > cache_t;
static __thread cache_t* cache;
static __thread bool init;

cache_t& get_cache() {
  if (init) {
    return *cache;
  } else {
    init = true;
    cache = new cache_t();
    return *cache;
  }
}

void hash_work(util::lang::shared_ptr<hash_task> desc) {
  const sfv_t& sfv = desc->sfv;
  const size_t fv_offset = desc->fv_offset;
  const size_t fv_length = desc->fv_length;
  const size_t hash_num = desc->hash_num;
  vector<float>& result = desc->result;

  for (size_t i = fv_offset; i < fv_offset + fv_length; ++i) {
    cache_t& cache = get_cache();
    cache_t::const_iterator it = cache.find(sfv[i].first);
    if (it != cache.end()) {
      // cache hit
      const vector<float>& random_vector = it->second;
      const float value = sfv[i].second;
      for (uint32_t j = 0; j < hash_num; j+=4) {
        float* target = &result[j];
        const float* mult = &random_vector[j];
        *target++ += value * *mult++;
        *target++ += value * *mult++;
        *target++ += value * *mult++;
        *target++ += value * *mult++;
      }
    } else {
      // cache miss-hit
      vector<float> random_vector;
      random_vector.reserve(hash_num);
      const uint32_t seed = common::hash_util::calc_string_hash(sfv[i].first);
      jubatus::util::math::random::mtrand rnd(seed);
      for (uint32_t j = 0; j < hash_num; ++j) {
        const float random = rnd.next_gaussian();
        result[j] += sfv[i].second * random;
        random_vector.push_back(random);
      }
      cache.insert(std::make_pair(sfv[i].first, random_vector));
    }
  }
  __sync_synchronize();
  desc->finished = true;
}

void lsh_hash_worker::hash(
    const sfv_t& fv,
    uint32_t hash_num,
    storage::bit_vector& result) {
  std::vector<util::lang::shared_ptr<hash_task> > tasks;
  if (fv.size() == 0) {
    result = bit_vector();
    return;
  }

  const size_t total_hash = hash_num * fv.size();
  const size_t jobs = total_hash / 100000;
  const size_t chunk = (fv.size() + jobs - 1) / jobs;
  for (uint64_t i = 0; i < fv.size(); i += chunk) {
    uint64_t til = std::min(fv.size(), i + chunk);
    util::lang::shared_ptr<hash_task> new_task
      (new hash_task(fv, i, til - i, hash_num));
    tasks.push_back(new_task);
    workers_.add_task(new_task, &hash_work);
  }

  std::vector<float> result_f(hash_num);
  for (size_t i = 0; i < tasks.size(); ++i) {
    __sync_synchronize();
    if (!tasks[i]->is_finished()) {
      --i;
      detail::yield();
      continue;
    }
    std::vector<float>& ret = tasks[i]->result;
    for (size_t i = 0; i < ret.size(); ++i) {
      result_f[i] += ret[i];
    }
  }
  storage::bit_vector result_bin(binarize(result_f));
  result.swap(result_bin);
}

}  // namespace nearest_neighbor
}  // namespace core
}  // namespace jubatus
