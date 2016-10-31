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

#include "dbscan_clustering_method.hpp"

#include <vector>
#include "../common/exception.hpp"
#include "clustering.hpp"
#include "util.hpp"

using std::pair;
using std::vector;

namespace jubatus {
namespace core {
namespace clustering {

dbscan_clustering_method::dbscan_clustering_method(
    double eps,
    size_t min_core_point)
    : eps_(eps),
      min_core_point_(min_core_point),
      dbscan_(eps, min_core_point) {
  if (!(0 < eps)) {
    throw JUBATUS_EXCEPTION(
                common::invalid_parameter("0 < eps"));
  }
  if (!(1 <= min_core_point)) {
    throw JUBATUS_EXCEPTION(
                common::invalid_parameter("0 < min_core_point"));
  }
}

dbscan_clustering_method::~dbscan_clustering_method() {
}

void dbscan_clustering_method::batch_update(wplist points) {
  if (points.empty()) {
    *this = dbscan_clustering_method(eps_, min_core_point_);
    return;
  }
  dbscan_.batch(points);
}

void dbscan_clustering_method::online_update(wplist points) {
}

std::vector<common::sfv_t> dbscan_clustering_method::get_k_center() const {
  throw JUBATUS_EXCEPTION(core::common::unsupported_method("get_k_center"));
}

int64_t dbscan_clustering_method::get_nearest_center_index(
    const common::sfv_t& point) const {
  throw JUBATUS_EXCEPTION(
      core::common::unsupported_method("get_nearest_center_index"));
}

common::sfv_t dbscan_clustering_method::get_nearest_center(
    const common::sfv_t& point) const {
  throw JUBATUS_EXCEPTION(
      core::common::unsupported_method("get_nearest_center"));
}

wplist dbscan_clustering_method::get_cluster(
    const size_t cluster_id,
    const wplist& points) const {
  return get_clusters(points)[cluster_id];
}

std::vector<wplist> dbscan_clustering_method::get_clusters(
    const wplist& points) const {
  std::vector<wplist> clusters = dbscan_.get_clusters();
  if (clusters.empty()) {
    throw JUBATUS_EXCEPTION(not_performed());
  }
  return clusters;
}

}  // namespace clustering
}  // namespace core
}  // namespace jubatus

