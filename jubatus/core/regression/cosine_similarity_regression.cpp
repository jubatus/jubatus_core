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

#include <utility>
#include <map>
#include <string>
#include <vector>

#include "cosine_similarity_regression.hpp"
#include "nearest_neighbor_regression_util.hpp"

using jubatus::util::lang::shared_ptr;
using jubatus::util::concurrent::scoped_lock;

namespace jubatus {
namespace core {
namespace regression {

cosine_similarity_regression::cosine_similarity_regression(
    const config& conf): inverted_index_regression(conf) {
}

float cosine_similarity_regression::estimate(
    const common::sfv_t& fv) const {
  std::vector<std::pair<std::string, float> > ids;
  {
    util::concurrent::scoped_rlock lk(storage_mutex_);
    mixable_storage_->get_model()->calc_scores(
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
         // The range of cosine similarity score is [-1.0, 1.0].
         float d = (1.0 - it->second) / 2.0;
         if (d == 0.0) {
           // In case distance equals zero, returns the vector's target value.
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

std::string cosine_similarity_regression::name() const {
    return "cosine similarity regression";
}

}  // namespace regression
}  // namespace core
}  // namespace jubatus
