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

#include "euclidean_distance_regression.hpp"
#include <string>
#include <utility>
#include <map>
#include <vector>

#include "jubatus/util/concurrent/lock.h"
#include "nearest_neighbor_regression_util.hpp"

using jubatus::util::lang::shared_ptr;
using jubatus::util::concurrent::scoped_lock;

namespace jubatus {
namespace core {
namespace regression {

euclidean_distance_regression::euclidean_distance_regression(
    const config& conf) : inverted_index_regression(conf) {
}

float euclidean_distance_regression::estimate(
    const common::sfv_t& fv) const {
  std::vector<std::pair<std::string, float> > ids;
  {
    util::concurrent::scoped_rlock lk(storage_mutex_);
    mixable_storage_->get_model()->calc_euclid_scores(
        fv, ids, config_.nearest_neighbor_num);
  }
  if (ids.size() > 0) {
    float sum = 0.0;
    if (config_.weight && *config_.weight == "distance") {
      float sum_w = 0.0;
      for (std::vector<std::pair<std::string, float> >:: const_iterator
               it = ids.begin(); it != ids.end(); ++it) {
        const std::pair<bool, uint64_t> index =
            values_->get_model()->exact_match(it->first);
        float t = values_->get_model()->get_float_column(0)[index.second];
        // calc_euclid_scores(...) returns minus euclid distance.
        float d = -1.0 * it->second;
        if (d == 0.0) {
          return t;
        } else {
          float w = 1.0 / d;
          sum += w * t;
          sum_w += w;
        }
      }
      return sum / sum_w;
    } else {
      for (std::vector<std::pair<std::string, float> >:: const_iterator
               it = ids.begin(); it != ids.end(); ++it) {
        const std::pair<bool, uint64_t> index =
            values_->get_model()->exact_match(it->first);
        sum += values_->get_model()->get_float_column(0)[index.second];
    }
      return sum / ids.size();
    }
  } else {
    return 0;
  }
}

std::string euclidean_distance_regression::name() const {
  return "euclidean distance regression";
}

}  // namespace regression
}  // namespace core
}  // namespace jubatus
