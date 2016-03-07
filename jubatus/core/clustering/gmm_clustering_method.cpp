// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2013 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#include "gmm_clustering_method.hpp"

#include <utility>
#include <vector>
#include "../common/exception.hpp"
#include "clustering.hpp"
#include "util.hpp"

using std::vector;

namespace jubatus {
namespace core {
namespace clustering {

gmm_clustering_method::gmm_clustering_method(size_t k, uint32_t seed)
    : k_(k), seed_(seed), kcenters_(), mapper_(), gmm_(seed) {
}

gmm_clustering_method::~gmm_clustering_method() {
}

void gmm_clustering_method::batch_update(wplist points) {
  if (points.empty()) {
    *this = gmm_clustering_method(k_, seed_);
    return;
  }
  mapper_.clear();
  eigen_wsvec_list_t data = mapper_.convert(points, true);
  gmm_.batch(data, mapper_.get_dimension(), k_);
  kcenters_ = mapper_.revert(gmm_.get_centers());
}

void gmm_clustering_method::online_update(wplist points) {
}

std::vector<common::sfv_t> gmm_clustering_method::get_k_center() const {
  if (kcenters_.empty()) {
    throw JUBATUS_EXCEPTION(not_performed());
  }

  return kcenters_;
}

int64_t gmm_clustering_method::get_nearest_center_index(
    const common::sfv_t& point) const {
  return gmm_.get_nearest_center_index(mapper_.convertc(point));
}

common::sfv_t gmm_clustering_method::get_nearest_center(
    const common::sfv_t& point) const {
  return mapper_.revert(gmm_.get_nearest_center(mapper_.convertc(point)));
}

wplist gmm_clustering_method::get_cluster(
    size_t cluster_id,
    const wplist& points) const {
  return get_clusters(points)[cluster_id];
}

std::vector<wplist> gmm_clustering_method::get_clusters(
    const wplist& points) const {
  if (kcenters_.empty()) {
    throw JUBATUS_EXCEPTION(not_performed());
  }

  std::vector<wplist> ret(k_);
  for (wplist::const_iterator it = points.begin(); it != points.end(); ++it) {
    int64_t c = get_nearest_center_index(it->data);
    ret[c].push_back(*it);
  }
  return ret;
}

}  // namespace clustering
}  // namespace core
}  // namespace jubatus
