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

#include "dbscan.hpp"

#include <vector>
#include "clustering.hpp"
#include "util.hpp"
#include "jubatus/util/lang/cast.h"
#include "../common/exception.hpp"

using std::vector;
using jubatus::util::lang::lexical_cast;


namespace jubatus {
namespace core {
namespace clustering {

dbscan::dbscan(double eps, size_t min_core_point)
    : eps_(eps),
      min_core_point_(min_core_point) {
}

void dbscan::batch(const wplist& points) {
  clusters_.clear();
  point_states_.assign(lexical_cast<int>(points.size()), dbscan::UNCLASSIFIED);

  for (size_t size = points.size(), i = 0; i < size; ++i) {
    if (point_states_[i] != dbscan::UNCLASSIFIED) {
      continue;
    }
    wplist cluster = dbscan::expand_cluster(i, points);

    if (cluster.size() >= min_core_point_) {
      clusters_.push_back(cluster);
    }
  }
}

std::vector<int> dbscan::get_point_states() const {
  return point_states_;
}

std::vector<wplist> dbscan::get_clusters() const {
  return clusters_;
}

void dbscan::set_eps(double eps) {
  eps_ = eps;
}

// return cluster as wplist
wplist dbscan::expand_cluster(const size_t idx, const wplist& points) {
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

std::vector<size_t> dbscan::region_query(
    const size_t idx, const wplist& points) const {
  std::vector<size_t> region;
  for (wplist::const_iterator it = points.begin(); it != points.end(); ++it) {
    if (dist((*it).data, points[idx].data) < eps_) {
      region.push_back(std::distance(points.begin(), it));
    }
  }
  return region;
}

}  // namespace clustering
}  // namespace core
}  // namespace jubatus
