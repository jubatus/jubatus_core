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

#include "exp3.hpp"

#include <string>
#include <vector>
#include "../common/exception.hpp"
#include "../common/version.hpp"
#include "../framework/packer.hpp"
#include "select_by_weights.hpp"

namespace jubatus {
namespace core {
namespace bandit {

exp3::exp3(bool assume_unrewarded, double gamma)
    : gamma_(gamma), s_(assume_unrewarded) {
  if (gamma < 0 || 1 < gamma) {
    throw JUBATUS_EXCEPTION(
        common::invalid_parameter("0 <= gamma <= 1"));
  }
}

void exp3::calc_weights_(const std::string& player_id,
                         std::vector<double>& weights) const {
  const std::vector<std::string>& arms = s_.get_arm_ids();
  if (arms.empty()) {
    throw JUBATUS_EXCEPTION(
        common::exception::runtime_error("arm is not registered"));
  }

  const size_t n = arms.size();
  weights.clear();
  weights.reserve(n);
  double total_weight = 0;
  for (size_t i = 0; i < n; ++i) {
    const double weight = std::exp(s_.get_arm_info(player_id, arms[i]).weight);
    weights.push_back(weight);
    total_weight += weight;
  }
  for (size_t i = 0; i < n; ++i) {
    weights[i] = (1.0 - gamma_) * weights[i] / total_weight + gamma_ * n;
  }
}

std::string exp3::select_arm(const std::string& player_id) {
  const std::vector<std::string>& arms = s_.get_arm_ids();
  if (arms.empty()) {
    throw JUBATUS_EXCEPTION(
        common::exception::runtime_error("arm is not registered"));
  }

  std::vector<double> weights;
  calc_weights_(player_id, weights);
  std::string result = arms[select_by_weights(weights, rand_)];
  s_.notify_selected(player_id, result);
  return result;
}

bool exp3::register_arm(const std::string& arm_id) {
  return s_.register_arm(arm_id);
}
bool exp3::delete_arm(const std::string& arm_id) {
  return s_.delete_arm(arm_id);
}

bool exp3::register_reward(const std::string& player_id,
                           const std::string& arm_id,
                           double reward) {
  const std::vector<std::string>& arms = s_.get_arm_ids();
  size_t i = std::find(arms.begin(), arms.end(), arm_id) - arms.begin();
  if (i >= arms.size()) {
    return false;
  }
  std::vector<double> weights;
  calc_weights_(player_id, weights);
  return s_.register_reward(player_id, arm_id,
                            reward * weights[i] * gamma_ / arms.size());
}

arm_info_map exp3::get_arm_info(const std::string& player_id) const {
  return s_.get_arm_info_map(player_id);
}

bool exp3::reset(const std::string& player_id) {
  return s_.reset(player_id);
}
void exp3::clear() {
  s_.clear();
}

void exp3::pack(framework::packer& pk) const {
  pk.pack(s_);
}
void exp3::unpack(msgpack::object o) {
  o.convert(&s_);
}

void exp3::get_diff(diff_t& diff) const {
  s_.get_diff(diff);
}
bool exp3::put_diff(const diff_t& diff) {
  return s_.put_diff(diff);
}
void exp3::mix(const diff_t& lhs, diff_t& rhs) const {
  s_.mix(lhs, rhs);
}

storage::version exp3::get_version() const {
  return storage::version();
}

}  // namespace bandit
}  // namespace core
}  // namespace jubatus
