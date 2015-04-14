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

#include "softmax.hpp"

#include <string>
#include <vector>
#include <cmath>
#include <numeric>
#include "../common/exception.hpp"
#include "../framework/packer.hpp"
#include "select_by_weights.hpp"
#include "../common/version.hpp"

namespace jubatus {
namespace core {
namespace bandit {

softmax::softmax(bool assume_unrewarded, double tau)
    : tau_(tau), s_(assume_unrewarded) {
  if (tau <= 0) {
    throw JUBATUS_EXCEPTION(
        common::invalid_parameter("0 < tau"));
  }
}

std::string softmax::select_arm(const std::string& player_id) {
  const std::vector<std::string>& arms = s_.get_arm_ids();
  if (arms.empty()) {
    throw JUBATUS_EXCEPTION(
        common::exception::runtime_error("arm is not registered"));
  }

  std::vector<double> weights;
  weights.reserve(arms.size());

  for (size_t i = 0; i < arms.size(); ++i) {
    double expectation = s_.get_expectation(player_id, arms[i]);
    weights.push_back(std::exp(expectation / tau_));
  }
  std::string result = arms[select_by_weights(weights, rand_)];
  s_.notify_selected(player_id, result);
  return result;
}

bool softmax::register_arm(const std::string& arm_id) {
  return s_.register_arm(arm_id);
}
bool softmax::delete_arm(const std::string& arm_id) {
  return s_.delete_arm(arm_id);
}

bool softmax::register_reward(const std::string& player_id,
                                     const std::string& arm_id,
                                     double reward) {
  return s_.register_reward(player_id, arm_id, reward);
}

arm_info_map softmax::get_arm_info(const std::string& player_id) const {
  return s_.get_arm_info_map(player_id);
}

bool softmax::reset(const std::string& player_id) {
  return s_.reset(player_id);
}
void softmax::clear() {
  s_.clear();
}

void softmax::pack(framework::packer& pk) const {
  pk.pack(s_);
}
void softmax::unpack(msgpack::object o) {
  o.convert(&s_);
}

void softmax::get_diff(diff_t& diff) const {
  s_.get_diff(diff);
}
bool softmax::put_diff(const diff_t& diff) {
  return s_.put_diff(diff);
}
void softmax::mix(const diff_t& lhs, diff_t& rhs) const {
  s_.mix(lhs, rhs);
}

storage::version softmax::get_version() const {
  return storage::version();
}

}  // namespace bandit
}  // namespace core
}  // namespace jubatus
