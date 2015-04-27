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

#include "epsilon_greedy.hpp"

#include <string>
#include <vector>
#include "../common/exception.hpp"
#include "../framework/packer.hpp"
#include "../common/version.hpp"

namespace jubatus {
namespace core {
namespace bandit {

epsilon_greedy::epsilon_greedy(bool assume_unrewarded, double eps)
    : eps_(eps), s_(assume_unrewarded) {
  if (eps < 0 || 1 < eps) {
    throw JUBATUS_EXCEPTION(
        common::invalid_parameter("0 <= epsilon <= 1"));
  }
}

std::string epsilon_greedy::select_arm(const std::string& player_id) {
  const std::vector<std::string>& arms = s_.get_arm_ids();
  if (arms.empty()) {
    throw JUBATUS_EXCEPTION(
        common::exception::runtime_error("arm is not registered"));
  }

  std::string result;
  if (rand_.next_double() < eps_) {
    // exploration
    result = arms[rand_.next_int(arms.size())];
  } else {
    // exploitation
    result = arms[0];
    double exp_max = s_.get_expectation(player_id, arms[0]);
    for (size_t i = 1; i < arms.size(); ++i) {
      double exp = s_.get_expectation(player_id, arms[i]);
      if (exp > exp_max) {
        result = arms[i];
        exp_max = exp;
      }
    }
  }
  s_.notify_selected(player_id, result);
  return result;
}

bool epsilon_greedy::register_arm(const std::string& arm_id) {
  return s_.register_arm(arm_id);
}
bool epsilon_greedy::delete_arm(const std::string& arm_id) {
  return s_.delete_arm(arm_id);
}

bool epsilon_greedy::register_reward(const std::string& player_id,
                                     const std::string& arm_id,
                                     double reward) {
  return s_.register_reward(player_id, arm_id, reward);
}

arm_info_map epsilon_greedy::get_arm_info(const std::string& player_id) const {
  return s_.get_arm_info_map(player_id);
}

bool epsilon_greedy::reset(const std::string& player_id) {
  return s_.reset(player_id);
}
void epsilon_greedy::clear() {
  s_.clear();
}

void epsilon_greedy::pack(framework::packer& pk) const {
  pk.pack(s_);
}
void epsilon_greedy::unpack(msgpack::object o) {
  o.convert(&s_);
}

void epsilon_greedy::get_diff(diff_t& diff) const {
  s_.get_diff(diff);
}
bool epsilon_greedy::put_diff(const diff_t& diff) {
  return s_.put_diff(diff);
}
void epsilon_greedy::mix(const diff_t& lhs, diff_t& rhs) const {
  s_.mix(lhs, rhs);
}

storage::version epsilon_greedy::get_version() const {
  return storage::version();
}

}  // namespace bandit
}  // namespace core
}  // namespace jubatus
