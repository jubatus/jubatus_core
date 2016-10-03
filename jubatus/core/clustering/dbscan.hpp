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

#ifndef JUBATUS_CORE_CLUSTERING_DBSCAN_HPP_
#define JUBATUS_CORE_CLUSTERING_DBSCAN_HPP_

#include <vector>
#include "types.hpp"

namespace jubatus {
namespace core {
namespace clustering {

class dbscan {
 public:
  dbscan(double eps, size_t min_core_point);

  void batch(const wplist& points);
  std::vector<int> get_point_states() const;
  std::vector<wplist> get_clusters() const;
  void set_eps(double eps);

 private:
  wplist expand_cluster(const size_t idx, const wplist& points);
  std::vector<size_t> region_query(
      const size_t idx, const wplist& points) const;

  double eps_;
  size_t min_core_point_;
  std::vector<int> point_states_;
  std::vector<wplist> clusters_;

  static const int UNCLASSIFIED = 0;
  static const int CLASSIFIED = 1;
  static const int NOISE = -1;
};

}  // namespace clustering
}  // namespace core
}  // namesapce jubatus

#endif  // JUBATUS_CORE_CLUSTERING_DBSCAN_HPP_
