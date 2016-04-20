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

#include "euclidean_distance_classifier.hpp"
#include <string>
#include <utility>
#include <map>
#include <vector>

#include "jubatus/util/concurrent/lock.h"
#include "nearest_neighbor_classifier_util.hpp"

using jubatus::util::lang::shared_ptr;
using jubatus::util::data::unordered_set;
using jubatus::util::concurrent::scoped_lock;

namespace jubatus {
namespace core {
namespace classifier {

euclidean_distance_classifier::euclidean_distance_classifier(
    size_t k,
    float alpha) : inverted_index_classifier(k, alpha) {
}

void euclidean_distance_classifier::classify_with_scores(
    const common::sfv_t& fv,
    classify_result& scores) const {
  std::vector<std::pair<std::string, float> > ids;
  mixable_storage_->get_model()->calc_euclid_scores(fv, ids, k_);

  std::map<std::string, float> m;
  {
    util::concurrent::scoped_lock lk(label_mutex_);
    for (unordered_set<std::string>::const_iterator iter = labels_.begin();
         iter != labels_.end(); ++iter) {
      m.insert(std::make_pair(*iter, 0));
    }
  }
  for (size_t i = 0; i < ids.size(); ++i) {
    std::string label = get_label_from_id(ids[i].first);
    m[label] += std::exp(-alpha_ * ids[i].second);
  }

  scores.clear();
  for (std::map<std::string, float>::const_iterator iter = m.begin();
       iter != m.end(); ++iter) {
    classify_result_elem elem(iter->first, iter->second);
    scores.push_back(elem);
  }
}

std::string euclidean_distance_classifier::name() const {
  return "cosine similarity classifier";
}

}  // namespace classifier
}  // namespace core
}  // namespace jubatus
