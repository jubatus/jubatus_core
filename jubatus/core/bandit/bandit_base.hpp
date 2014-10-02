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

#ifndef JUBATUS_CORE_BANDIT_BANDIT_BASE_HPP_
#define JUBATUS_CORE_BANDIT_BANDIT_BASE_HPP_

#include <string>
#include <vector>

#include "registered_reward.hpp"
#include "storage.hpp"

namespace jubatus {
namespace core {
namespace bandit {

class bandit_base {
 public:
  bandit_base();
  virtual ~bandit_base();

  bool register_arm(const std::string& arm_id);
  bool delete_arm(const std::string& arm_id);

  virtual std::string select_arm(const std::string& player_id) = 0;

  void register_reward(const std::string& player_id,
                       const std::string& arm_id,
                       double reward);

  registered_rewards get_registered_rewards(const std::string& player_id) const;

  bool reset(const std::string& player_id);
  void clear();

  virtual std::string name() const = 0;

  const std::vector<std::string>& get_arms() const {
    return arms_;
  }
  const storage& get_storage() const {
    return s_;
  }

 private:
  std::vector<std::string> arms_;
  storage s_;
};

}  // namespace bandit
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_BANDIT_BANDIT_BASE_HPP_
