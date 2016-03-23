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

#include <vector>
#include "jubatus/util/math/random.h"
#include "../common/hash.hpp"
#include "../common/type.hpp"
#include "../storage/bit_vector.hpp"
#include "lsh_function.hpp"
#include "jubatus/util/data/unordered_map.h"

using std::vector;
using jubatus::core::storage::bit_vector;

namespace jubatus {
namespace core {
namespace nearest_neighbor {

typedef util::data::unordered_map<uint32_t, vector<double> > cache_t;
static cache_t cache;

vector<float> random_projection(const common::sfv_t& sfv, uint32_t hash_num) {
  vector<float> proj(hash_num);
  for (size_t i = 0; i < sfv.size(); ++i) {
    const uint32_t seed = common::hash_util::calc_string_hash(sfv[i].first);
    cache_t::const_iterator it = cache.find(seed);
    if (0 and it != cache.end()) { // TODO(yuhara) cache disable. because of test(neighbor_row_and_similar_row) failed
      // cache hit
      const vector<double>& random_vector = it->second;
      for (uint32_t j = 0; j < hash_num; ++j) {
        proj[j] += sfv[i].second * random_vector[j];
      }
    } else {
      // cache miss-hit
      vector<double> random_vector;
      random_vector.reserve(hash_num);
      jubatus::util::math::random::mtrand rnd(seed);
      for (uint32_t j = 0; j < hash_num; ++j) {
        const double random = rnd.next_gaussian();
        proj[j] += sfv[i].second * random;
        random_vector.push_back(random);
      }
      cache.insert(std::make_pair(seed, random_vector));
    }
  }

  return proj;
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

}  // namespace nearest_neighbor
}  // namespace core
}  // namespace jubatus
