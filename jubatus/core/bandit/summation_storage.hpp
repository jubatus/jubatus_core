// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2015 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#ifndef JUBATUS_CORE_BANDIT_SUMMATION_STORAGE_HPP_
#define JUBATUS_CORE_BANDIT_SUMMATION_STORAGE_HPP_

#include <string>
#include <vector>


#include "bandit_base.hpp"


namespace jubatus {
namespace core {
namespace bandit {

class summation_storage {
 public:
  typedef bandit_base::diff_t table_t;

  explicit summation_storage(bool assume_unrewarded);

  bool register_arm(const std::string& arm_id);
  bool delete_arm(const std::string& arm_id);

  void notify_selected(const std::string& player_id,
                       const std::string& arm_id);
  bool register_reward(const std::string& player_id,
                       const std::string& arm_id,
                       double reward);

  arm_info get_arm_info(const std::string& player_id,
                        const std::string& arm_id) const;
  double get_expectation(const std::string& player_id,
                         const std::string& arm_id) const;
  int get_total_trial_count(const std::string& player_id) const;
  const std::vector<std::string>& get_arm_ids() const {
    return arm_ids_;
  }
  arm_info_map get_arm_info_map(const std::string& player_id) const;

  void get_diff(table_t& diff) const;
  bool put_diff(const table_t& diff);
  static void mix(const table_t& lhs, table_t& rhs);

  bool reset(const std::string& player_id);
  void clear();

  MSGPACK_DEFINE(arm_ids_, mixed_, unmixed_);

 private:
  const bool assume_unrewarded_;
  std::vector<std::string> arm_ids_;
  table_t mixed_, unmixed_;
};

}  // namespace bandit
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_BANDIT_SUMMATION_STORAGE_HPP_
