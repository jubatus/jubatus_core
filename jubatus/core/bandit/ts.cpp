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

#include "ts.hpp"

#include <string>
#include <vector>
#include <cfloat>
#include "../common/exception.hpp"
#include "select_by_weights.hpp"

namespace jubatus {
namespace core {
namespace bandit {

using jubatus::util::math::random::mtrand;

ts::ts() {
}

std::string ts::select_arm(const std::string& player_id) {
  const std::vector<std::string>& arms = s_.get_arm_ids();
  if (arms.empty()) {
    throw JUBATUS_EXCEPTION(
        common::exception::runtime_error("arm is not registered"));
  }

  double score_max = -DBL_MAX;
  std::string result;
  for (size_t i = 0; i < arms.size(); ++i) {
    const arm_info& a = s_.get_arm_info(player_id, arms[i]);
    double alpha = a.weight + 1.0;
    double beta = a.trial_count - a.weight + 1.0;
    double score = rand_.next_beta(alpha, beta);
    if (score > score_max) {
      score_max = score;
      result = arms[i];
    }
  }
  return result;
  //return arms[select_by_weights(weights, rand_)];
}

bool ts::register_arm(const std::string& arm_id) {
  return s_.register_arm(arm_id);
}
bool ts::delete_arm(const std::string& arm_id) {
  return s_.delete_arm(arm_id);
}

bool ts::register_reward(const std::string& player_id,
                           const std::string& arm_id,
                           double reward) {
  if( (reward != 0.0) && (reward != 1.0) ){
    throw JUBATUS_EXCEPTION(
        common::exception::runtime_error("reward is not in {0,1}")); //Thompson sampling assumes binary rewards
  }
  const std::vector<std::string>& arms = s_.get_arm_ids();
  size_t i = std::find(arms.begin(), arms.end(), arm_id) - arms.begin();
  if (i >= arms.size()) {
    return false;
  }
  return s_.register_reward(player_id, arm_id, reward);
}

arm_info_map ts::get_arm_info(const std::string& arm_id) const {
  return s_.get_arm_info_map(arm_id);
}

bool ts::reset(const std::string& player_id) {
  return s_.reset(player_id);
}
void ts::clear() {
  s_.clear();
}

void ts::pack(framework::packer& pk) const {
  pk.pack(s_);
}
void ts::unpack(msgpack::object o) {
  o.convert(&s_);
}

void ts::get_diff(diff_t& diff) const {
  s_.get_diff(diff);
}
bool ts::put_diff(const diff_t& diff) {
  return s_.put_diff(diff);
}
void ts::mix(const diff_t& lhs, diff_t& rhs) const {
  s_.mix(lhs, rhs);
}

}  // namespace bandit
}  // namespace core
}  // namespace jubatus
