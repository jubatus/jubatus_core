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

#ifndef JUBATUS_CORE_ANOMALY_LIGHT_LOF_HPP_
#define JUBATUS_CORE_ANOMALY_LIGHT_LOF_HPP_

#include <string>
#include <utility>
#include <vector>
#include "jubatus/util/data/serialization.h"
#include "jubatus/util/data/unordered_set.h"
#include "jubatus/util/data/optional.h"
#include "jubatus/util/lang/shared_ptr.h"
#include "anomaly_base.hpp"

namespace jubatus {
namespace core {
namespace storage {
class column_table;
}  // namespace storage
namespace unlearner {
class unlearner_base;
}  // namespace unlearner
namespace nearest_neighbor {
class nearest_neighbor_base;
}  // namespace nearest_neighbor
namespace framework {
class mixable_versioned_table;
}  // namespace framework
namespace anomaly {

// LOF implementation using nearest_neighbor as a backend.
class light_lof : public anomaly_base {
 public:
  // Configuration parameters of LOF.
  struct config {
    config();

    int nearest_neighbor_num;
    int reverse_nearest_neighbor_num;
    jubatus::util::data::optional<bool> ignore_kth_same_point;

    template<typename Ar>
    void serialize(Ar& ar) {
      ar
          & JUBA_MEMBER(nearest_neighbor_num)
          & JUBA_MEMBER(reverse_nearest_neighbor_num)
          & JUBA_MEMBER(ignore_kth_same_point);
    }
  };

  light_lof(
      const config& config,
      const std::string& id,
      jubatus::util::lang::shared_ptr<nearest_neighbor::nearest_neighbor_base>
          nearest_neighbor_engine);

  light_lof(
      const config& config,
      const std::string& id,
      jubatus::util::lang::shared_ptr<nearest_neighbor::nearest_neighbor_base>
          nearest_neighbor_engine,
      jubatus::util::lang::shared_ptr<unlearner::unlearner_base> unlearner);

  virtual ~light_lof();

  // anomaly_base interface

  float calc_anomaly_score(const common::sfv_t& query) const;
  float calc_anomaly_score(const std::string& id) const;
  // calc_anomaly_score(string, sfv_t) is not supported in light_lof
  float calc_anomaly_score(
      const std::string& id,
      const common::sfv_t& query) const;

  void clear();
  // clear_row is not supported
  void clear_row(const std::string& id);
  // update_row is not supported
  bool update_row(const std::string& id, const sfv_diff_t& diff);
  bool set_row(const std::string& id, const common::sfv_t& sfv);

  void get_all_row_ids(std::vector<std::string>& ids) const;
  std::string type() const;
  std::vector<framework::mixable*> get_mixables() const;

  void pack(framework::packer& packer) const;
  void unpack(msgpack::object o);

  bool is_updatable() const {
    return false;
  }

 private:
  // Parameters of each data point.
  struct parameter {
    float kdist;
    float lrd;
  };

  void touch(const std::string& id);
  void unlearn(const std::string& id);

  float collect_lrds(
      const common::sfv_t& query,
      std::vector<float>& neighbor_lrds) const;
  float collect_lrds(
      const std::string& query,
      std::vector<float>& neighbor_lrds) const;
  float collect_lrds_from_neighbors(
      const std::vector<std::pair<std::string, float> >& neighbors,
      std::vector<float>& neighbor_lrd) const;

  void collect_neighbors(
      const std::string& query,
      jubatus::util::data::unordered_set<std::string>& neighbors) const;
  void update_entries(
      const jubatus::util::data::unordered_set<std::string>& neighbors);

  // Gets parameters of given row. If row does not exist, it throws an
  // exception.
  parameter get_row_parameter(const std::string& row) const;

  jubatus::util::lang::shared_ptr<nearest_neighbor::nearest_neighbor_base>
      nearest_neighbor_engine_;
  jubatus::util::lang::shared_ptr<unlearner::unlearner_base> unlearner_;

  // Mixable of nearest neighbor model.
  jubatus::util::lang::shared_ptr<framework::mixable_versioned_table>
      mixable_nearest_neighbor_;
  // Mixable of score table that contains k-dists and LRDs.
  jubatus::util::lang::shared_ptr<framework::mixable_versioned_table>
      mixable_scores_;

  config config_;
  std::string my_id_;
};

}  // namespace anomaly
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_ANOMALY_LIGHT_LOF_HPP_
