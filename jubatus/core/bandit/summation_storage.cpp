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

#include "summation_storage.hpp"

#include <string>
#include <vector>

namespace jubatus {
namespace core {
namespace bandit {

summation_storage::summation_storage() {
}

bool summation_storage::register_arm(const std::string& arm_id) {
  if (std::find(arm_ids_.begin(), arm_ids_.end(), arm_id) != arm_ids_.end()) {
    // arm_id is already in arms_
    return false;
  }
  arm_ids_.push_back(arm_id);
  return true;
}

namespace {
void delete_arm_(summation_storage::table_t& t, const std::string& arm_id) {
  for (summation_storage::table_t::iterator iter = t.begin();
       iter != t.end(); ++iter) {
    arm_info_map& as = iter->second;
    as.erase(arm_id);
  }
}
}  // namespace

bool summation_storage::delete_arm(const std::string& arm_id) {
  delete_arm_(mixed_, arm_id);
  delete_arm_(unmixed_, arm_id);

  std::vector<std::string>::iterator iter =
      std::remove(arm_ids_.begin(), arm_ids_.end(), arm_id);
  if (iter == arm_ids_.end()) {
    return false;
  }
  arm_ids_.erase(iter, arm_ids_.end());
  return true;
}

bool summation_storage::register_reward(
    const std::string& player_id,
    const std::string& arm_id,
    double reward) {
  arm_info_map& as = unmixed_[player_id];
  arm_info& a = as[arm_id];
  a.trial_count += 1;
  a.weight += reward;
  return true;
}

namespace {
arm_info get_arm_info_(
    const summation_storage::table_t& t,
    const std::string& player_id,
    const std::string& arm_id) {
  summation_storage::table_t::const_iterator iter = t.find(player_id);
  if (iter == t.end()) {
    const arm_info a0 = {0, 0.0};
    return a0;
  }
  const arm_info_map& as = iter->second;
  arm_info_map::const_iterator jter = as.find(arm_id);
  if (jter == as.end()) {
    const arm_info a0 = {0, 0.0};
    return a0;
  }
  return jter->second;
}
}  // namespace

arm_info summation_storage::get_arm_info(
    const std::string& player_id,
    const std::string& arm_id) const {
  const arm_info a1 = get_arm_info_(mixed_, player_id, arm_id);
  const arm_info a2 = get_arm_info_(unmixed_, player_id, arm_id);

  arm_info result;
  result.trial_count = a1.trial_count + a2.trial_count;
  result.weight = a1.weight + a2.weight;
  return result;
}

double summation_storage::get_expectation(
    const std::string& player_id,
    const std::string& arm_id) const {
  const arm_info a = get_arm_info(player_id, arm_id);
  if (a.trial_count == 0) {
    return 0;
  }
  return a.weight / a.trial_count;
}

arm_info_map summation_storage::get_arm_info_map(
    const std::string& player_id) const {
  arm_info_map result;

  for (std::vector<std::string>::const_iterator iter = arm_ids_.begin();
       iter != arm_ids_.end(); ++iter) {
    result.insert(std::make_pair(*iter, get_arm_info(player_id, *iter)));
  }

  return result;
}

void summation_storage::get_diff(table_t& diff) const {
  diff = unmixed_;
}

bool summation_storage::put_diff(const table_t& diff) {
  mix(diff, mixed_);
  unmixed_.clear();
  return true;
}

void summation_storage::mix(const table_t& lhs, table_t& rhs) {
  for (table_t::const_iterator iter = lhs.begin();
       iter != lhs.end(); ++iter) {
    arm_info_map& as0 = rhs[iter->first];
    const arm_info_map& as1 = iter->second;
    for (arm_info_map::const_iterator jter = as1.begin();
         jter != as1.end(); ++jter) {
      arm_info& a0 = as0[jter->first];
      const arm_info& a1 = jter->second;
      a0.trial_count += a1.trial_count;
      a0.weight += a1.weight;
    }
  }
}

bool summation_storage::reset(const std::string& player_id) {
  bool result1 = mixed_.erase(player_id) > 0;
  bool result2 = unmixed_.erase(player_id) > 0;
  return result1 || result2;
}

void summation_storage::clear() {
  mixed_.clear();
  unmixed_.clear();
}

}  // namespace bandit
}  // namespace core
}  // namespace jubatus
