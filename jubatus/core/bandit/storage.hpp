// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2014 Preferred Infrastructure and Nippon Telegraph and Telephone Corporation.
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

#ifndef JUBATUS_CORE_BANDIT_STORAGE_HPP_
#define JUBATUS_CORE_BANDIT_STORAGE_HPP_

#include <string>

#include "registered_reward.hpp"
#include "../common/unordered_map.hpp"
#include "../common/version.hpp"

namespace jubatus {
namespace core {
namespace bandit {

class storage {
 public:
  typedef jubatus::util::data::unordered_map<std::string, registered_rewards>
      table_t;

  storage();

  void register_reward(const std::string& player_id,
                       const std::string& arm_id,
                       double reward);

  registered_reward get_registered_reward(const std::string& player_id,
                                          const std::string& arm_id) const;
  double get_expectation(const std::string& player_id,
                         const std::string& arm_id) const;

  void get_diff(table_t& diff) const;
  bool put_diff(const table_t& diff);
  static void mix(const table_t& lhs, table_t& rhs);

  void delete_arm(const std::string& arm_id);
  bool reset(const std::string& player_id);
  void clear();

  core::storage::version get_version() const {
    return core::storage::version();
  }

 private:
  table_t mixed_, unmixed_;
};

}  // namespace bandit
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_BANDIT_STORAGE_HPP_
