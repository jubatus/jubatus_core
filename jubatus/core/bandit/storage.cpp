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

#include "storage.hpp"

#include <string>

namespace jubatus {
namespace core {
namespace bandit {

storage::storage() {
}

void storage::register_reward(
    const std::string& player_id,
    const std::string& arm_id,
    double reward) {
  registered_rewards& rs = unmixed_[player_id];
  registered_reward& r = rs[arm_id];
  r.trial_count += 1;
  r.total_reward += reward;
}

namespace {
registered_reward get_registered_reward_(
    const storage::table_t& t,
    const std::string& player_id,
    const std::string& arm_id) {
  storage::table_t::const_iterator iter = t.find(player_id);
  if (iter == t.end()) {
    const registered_reward r0 = {0, 0.0};
    return r0;
  }
  const registered_rewards& rs = iter->second;
  registered_rewards::const_iterator jter = rs.find(arm_id);
  if (jter == rs.end()) {
    const registered_reward r0 = {0, 0.0};
    return r0;
  }
  return jter->second;
}
}  // namespace

registered_reward storage::get_registered_reward(
    const std::string& player_id,
    const std::string& arm_id) const {
  registered_reward r1 = get_registered_reward_(mixed_, player_id, arm_id);
  registered_reward r2 = get_registered_reward_(unmixed_, player_id, arm_id);

  registered_reward result;
  result.trial_count = r1.trial_count + r2.trial_count;
  result.total_reward = r1.total_reward + r2.total_reward;
  return result;
}

double storage::get_expectation(
    const std::string& player_id,
    const std::string& arm_id) const {
  registered_reward r = get_registered_reward(player_id, arm_id);
  if (r.trial_count == 0) {
    return 0;
  }
  return r.total_reward / r.trial_count;
}

void storage::get_diff(table_t& diff) const {
  diff = unmixed_;
}

bool storage::put_diff(const table_t& diff) {
  mix(diff, mixed_);
  unmixed_.clear();
  return true;
}

void storage::mix(const table_t& lhs, table_t& rhs) {
  for (table_t::const_iterator iter = lhs.begin();
       iter != lhs.end(); ++iter) {
    registered_rewards& rs0 = rhs[iter->first];
    const registered_rewards& rs1 = iter->second;
    for (registered_rewards::const_iterator jter = rs1.begin();
         jter != rs1.end(); ++jter) {
      registered_reward& r0 = rs0[jter->first];
      const registered_reward& r1 = jter->second;
      r0.trial_count += r1.trial_count;
      r0.total_reward += r1.total_reward;
    }
  }
}

namespace {
void delete_arm_(storage::table_t& t, const std::string& arm_id) {
  for (storage::table_t::iterator iter = t.begin();
       iter != t.end(); ++iter) {
    registered_rewards& rs = iter->second;
    rs.erase(arm_id);
  }
}
}  // namespace

void storage::delete_arm(const std::string& arm_id) {
  delete_arm_(mixed_, arm_id);
  delete_arm_(unmixed_, arm_id);
}

bool storage::reset(const std::string& player_id) {
  bool result1 = mixed_.erase(player_id) > 0;
  bool result2 = unmixed_.erase(player_id) > 0;
  return result1 || result2;
}

void storage::clear() {
  mixed_.clear();
  unmixed_.clear();
}

}  // namespace bandit
}  // namespace core
}  // namespace jubatus
