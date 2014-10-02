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

#include "bandit_base.hpp"

#include <string>
#include <vector>
#include <algorithm>

namespace jubatus {
namespace core {
namespace bandit {

bandit_base::bandit_base() {
}

bandit_base::~bandit_base() {
}

bool bandit_base::register_arm(const std::string& arm_id) {
  if (std::find(arms_.begin(), arms_.end(), arm_id) != arms_.end()) {
    // arm_id is already in arms_
    return false;
  }
  arms_.push_back(arm_id);
  return true;
}

bool bandit_base::delete_arm(const std::string& arm_id) {
  s_.delete_arm(arm_id);
  std::vector<std::string>::iterator iter =
      std::remove(arms_.begin(), arms_.end(), arm_id);
  if (iter == arms_.end()) {
    return false;
  }
  arms_.erase(iter, arms_.end());
  return true;
}

void bandit_base::register_reward(const std::string& player_id,
                                  const std::string& arm_id,
                                  double reward) {
  s_.register_reward(player_id, arm_id, reward);
}

registered_rewards bandit_base::get_registered_rewards(
    const std::string& player_id) const {
  registered_rewards result;

  for (std::vector<std::string>::const_iterator iter = arms_.begin();
       iter != arms_.end(); ++iter) {
    const std::string& arm_id = *iter;
    result.insert(
        std::make_pair(arm_id, s_.get_registered_reward(player_id, arm_id)));
  }

  return result;
}

bool bandit_base::reset(const std::string& player_id) {
  return s_.reset(player_id);
}

void bandit_base::clear() {
  s_.clear();
  arms_.clear();
}

}  // namespace bandit
}  // namespace core
}  // namespace jubatus
