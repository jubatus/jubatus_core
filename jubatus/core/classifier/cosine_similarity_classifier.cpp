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

#include "cosine_similarity_classifier.hpp"
#include "nearest_neighbor_classifier_util.hpp"
#include "classifier_type.hpp"

using jubatus::util::lang::shared_ptr;
using jubatus::util::concurrent::scoped_lock;

namespace jubatus {
namespace core {
namespace classifier {

cosine_similarity_classifier::cosine_similarity_classifier(
    size_t k,
    float alpha) : inverted_index_classifier(k, alpha) {
}

void cosine_similarity_classifier::classify_with_scores(
    const common::sfv_t& fv,
    classify_result& scores) const {
  std::vector<std::pair<std::string, float> > ids;
  {
    util::concurrent::scoped_rlock lk(storage_mutex_);
    mixable_storage_->get_model()->calc_scores(fv, ids, k_);
  }

  labels_t labels = labels_.get_model()->get_labels();
  std::map<std::string, float> m;
  for (labels_t::const_iterator iter = labels.begin();
       iter != labels.end(); ++iter) {
    m.insert(std::make_pair(iter->first, 0));
  }

  for (size_t i = 0; i < ids.size(); ++i) {
    std::string label = get_label_from_id(ids[i].first);
    m[label] += std::exp(-alpha_ * (1 - ids[i].second));
  }

  scores.clear();
  for (std::map<std::string, float>::const_iterator iter = m.begin();
       iter != m.end(); ++iter) {
    classify_result_elem elem(iter->first, iter->second);
    scores.push_back(elem);
  }
}

std::string cosine_similarity_classifier:: name() const {
  return "cosine similarity classifier";
}

}  // namespace classifier
}  // namespace core
}  // namespace jubatus
