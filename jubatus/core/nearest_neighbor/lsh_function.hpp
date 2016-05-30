// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2012 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#ifndef JUBATUS_CORE_NEAREST_NEIGHBOR_LSH_FUNCTION_HPP_
#define JUBATUS_CORE_NEAREST_NEIGHBOR_LSH_FUNCTION_HPP_

#include <stdint.h>
#include <vector>
#include "../common/lru.hpp"
#include "../common/type.hpp"
#include "../storage/bit_vector.hpp"
#include "jubatus/util/concurrent/mutex.h"
#include "jubatus/util/lang/scoped_ptr.h"

namespace jubatus {
namespace core {
namespace nearest_neighbor {

class random_projection_cache {
 public:
  explicit random_projection_cache(int size) : lru(size), lock() {}
  ~random_projection_cache() {}
  common::lru<uint32_t, std::vector<float> > lru;
  jubatus::util::concurrent::mutex lock;
};
typedef jubatus::util::lang::scoped_ptr<random_projection_cache> cache_t;

std::vector<float> random_projection(
    const common::sfv_t& sfv,
    uint32_t hash_num,
    uint32_t threads,
    cache_t& cache);
storage::bit_vector binarize(const std::vector<float>& proj);
storage::bit_vector cosine_lsh(
    const common::sfv_t& sfv,
    uint32_t hash_num,
    uint32_t threads,
    cache_t& cache);

template<typename T>
void init_cache_from_config(cache_t& cache, const T& config) {
  if (config.bool_test()) {
    if (!(0 <= *config)) {
      throw JUBATUS_EXCEPTION(common::invalid_parameter("0 <= cache_size"));
    }
    if (*config > 0) {
      cache.reset(new random_projection_cache(*config));
    }
  }
}

}  // namespace nearest_neighbor
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_NEAREST_NEIGHBOR_LSH_FUNCTION_HPP_
