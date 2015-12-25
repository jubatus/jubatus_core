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

#include "ucb1.hpp"

#include <string>
#include <vector>
#include <cfloat>
#include "../common/exception.hpp"
#include "../framework/packer.hpp"
#include "../common/version.hpp"

namespace jubatus {
namespace core {
namespace bandit {

ucb1::ucb1(bool assume_unrewarded)
    : s_(assume_unrewarded) {
}

std::string ucb1::select_arm(const std::string& player_id) {
  const std::vector<std::string>& arms = s_.get_arm_ids();
  if (arms.empty()) {
    throw JUBATUS_EXCEPTION(
        common::exception::runtime_error("arm is not registered"));
  }

  double log_total_trial = std::log(s_.get_total_trial_count(player_id));
  double score_max = -DBL_MAX;
  std::string result;
  for (size_t i = 0; i < arms.size(); ++i) {
    const arm_info& a = s_.get_arm_info(player_id, arms[i]);
    if (a.trial_count == 0) {
      s_.notify_selected(player_id, arms[i]);
      return arms[i];
    }
    double exp = a.weight / a.trial_count;
    double score = exp + std::sqrt(2 * log_total_trial / a.trial_count);
    if (score > score_max) {
      score_max = score;
      result = arms[i];
    }
  }
  s_.notify_selected(player_id, result);
  return result;
}

bool ucb1::register_arm(const std::string& arm_id) {
  return s_.register_arm(arm_id);
}
bool ucb1::delete_arm(const std::string& arm_id) {
  return s_.delete_arm(arm_id);
}

bool ucb1::register_reward(const std::string& player_id,
                                     const std::string& arm_id,
                                     double reward) {
  return s_.register_reward(player_id, arm_id, reward);
}

arm_info_map ucb1::get_arm_info(const std::string& player_id) const {
  return s_.get_arm_info_map(player_id);
}

bool ucb1::reset(const std::string& player_id) {
  return s_.reset(player_id);
}
void ucb1::clear() {
  s_.clear();
}

void ucb1::pack(framework::packer& pk) const {
  pk.pack(s_);
}
void ucb1::unpack(msgpack::object o) {
  o.convert(&s_);
}

void ucb1::get_diff(diff_t& diff) const {
  s_.get_diff(diff);
}
bool ucb1::put_diff(const diff_t& diff) {
  return s_.put_diff(diff);
}
void ucb1::mix(const diff_t& lhs, diff_t& rhs) const {
  s_.mix(lhs, rhs);
}
storage::version ucb1::get_version() const {
  return storage::version();
}

}  // namespace bandit
}  // namespace core
}  // namespace jubatus
