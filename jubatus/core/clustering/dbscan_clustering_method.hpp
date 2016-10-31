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

#ifndef JUBATUS_CORE_CLUSTERING_DBSCAN_CLUSTERING_METHOD_HPP_
#define JUBATUS_CORE_CLUSTERING_DBSCAN_CLUSTERING_METHOD_HPP_

#include <vector>
#include "../common/type.hpp"
#include "clustering_method.hpp"
#include "dbscan.hpp"

namespace jubatus {
namespace core {
namespace clustering {

class dbscan_clustering_method : public clustering_method {
 public:
  struct config {
    config()
      : eps(2.0), min_core_point(1) {
    }
    double eps;
    int min_core_point;

    MSGPACK_DEFINE(
        eps,
        min_core_point);

    template<typename Ar>
    void serialize(Ar& ar) {
      ar & JUBA_MEMBER(eps)
        & JUBA_MEMBER(min_core_point);
    }
  };

  dbscan_clustering_method(double eps, size_t min_core_point);
  ~dbscan_clustering_method();

  void batch_update(wplist points);
  void online_update(wplist points);
  std::vector<common::sfv_t> get_k_center() const;
  common::sfv_t get_nearest_center(const common::sfv_t& point) const;
  int64_t get_nearest_center_index(const common::sfv_t& point) const;
  wplist get_cluster(size_t cluster_id, const wplist& points) const;
  std::vector<wplist> get_clusters(const wplist& points) const;

 private:
  void initialize_centers(wplist& points);
  void do_batch_update(wplist& points);

  std::vector<common::sfv_t> kcenters_;
  double eps_;
  size_t min_core_point_;
  dbscan dbscan_;
};

}  // namespace clustering
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_CLUSTERING_DBSCAN_CLUSTERING_METHOD_HPP_
