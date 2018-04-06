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
#include "jubatus/util/lang/cast.h"

using std::pair;
using std::vector;
using jubatus::util::lang::lexical_cast;
namespace jubatus {
namespace core {
namespace clustering {

const int dbscan_clustering_method::UNCLASSIFIED = 0;
const int dbscan_clustering_method::CLASSIFIED = 1;
const int dbscan_clustering_method::NOISE = -1;

dbscan_clustering_method::dbscan_clustering_method(
    double eps,
    size_t min_core_point)
    : eps_(eps),
      min_core_point_(min_core_point) {
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
  update(points);
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
  if (clusters_.empty()) {
    throw JUBATUS_EXCEPTION(not_performed());
  }
  return clusters_;
}

void dbscan_clustering_method::update(const wplist& points) {
  clusters_.clear();
  point_states_.assign(lexical_cast<int>(points.size()), UNCLASSIFIED);

  for (size_t size = points.size(), i = 0; i < size; ++i) {
    if (point_states_[i] != UNCLASSIFIED) {
      continue;
    }
    wplist cluster = expand_cluster(i, points);

    if (cluster.size() >= min_core_point_) {
      clusters_.push_back(cluster);
    }
  }
}

std::vector<int> dbscan_clustering_method::get_point_states() const {
  return point_states_;
}


wplist dbscan_clustering_method::expand_cluster(
    const size_t idx,
    const wplist& points) {
  wplist cluster;
  vector<size_t> core = region_query(idx, points);

  if (core.size() < min_core_point_) {
    point_states_[idx] = NOISE;
  } else {
    point_states_[idx] = CLASSIFIED;
    cluster.push_back(points[idx]);
    do {
      std::vector<size_t> added_core;
      for (vector<size_t>::iterator it = core.begin(); it != core.end(); ++it) {
        if (point_states_[*it] == CLASSIFIED) {
          continue;
        }
        point_states_[*it] = CLASSIFIED;
        cluster.push_back(points[*it]);

        vector<size_t> expand_core = region_query((*it), points);
        for (vector<size_t>::iterator expand_it = expand_core.begin();
            expand_it != expand_core.end(); ++expand_it) {
          if (expand_core.size() >= min_core_point_) {
            if (point_states_[*expand_it] == CLASSIFIED) {
              continue;
            }
            if (point_states_[*expand_it] == UNCLASSIFIED) {
              // if point have not classified yet, the point push core.
              added_core.push_back(*expand_it);
            } else {
              cluster.push_back(points[*expand_it]);
              point_states_[*expand_it] = CLASSIFIED;
            }
          }
        }
      }
      core = added_core;
    } while (!core.empty());
  }
  return cluster;
}
  
std::vector<size_t> dbscan_clustering_method::region_query(
    const size_t idx, const wplist& points) const {
  std::vector<size_t> region;
  for (wplist::const_iterator it = points.begin(); it != points.end(); ++it) {
    if (sfv_dist_((*it).data, points[idx].data) < eps_) {
      region.push_back(std::distance(points.begin(), it));
    }
  }
  return region;
}

  
}  // namespace clustering
}  // namespace core
}  // namespace jubatus

