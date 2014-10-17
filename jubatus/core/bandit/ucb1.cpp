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

#include "ucb1.hpp"

#include <string>
#include <vector>
#include <cfloat>
#include "../common/exception.hpp"

namespace jubatus {
namespace core {
namespace bandit {

ucb1::ucb1(const jubatus::util::lang::shared_ptr<storage>& s)
    : bandit_base(s) {
}

std::string ucb1::select_arm(const std::string& player_id) {
  const std::vector<std::string>& arms = get_arms();
  if (arms.empty()) {
    throw JUBATUS_EXCEPTION(
        common::exception::runtime_error("arm is not registered"));
  }
  const storage& s = get_storage();

  int total_trial = 0;
  for (size_t i = 0; i < arms.size(); ++i) {
    const registered_reward& r = s.get_registered_reward(player_id, arms[i]);
    if (r.trial_count == 0) {
      return arms[i];
    }
    total_trial += r.trial_count;
  }
  double log_total_trial = std::log(total_trial);

  double score_max = -DBL_MAX;
  std::string result;
  for (size_t i = 0; i < arms.size(); ++i) {
    const registered_reward& r = s.get_registered_reward(player_id, arms[i]);
    double exp = r.total_reward / r.trial_count;
    double score = exp + std::sqrt(2 * log_total_trial / r.trial_count);
    if (score > score_max) {
      score_max = score;
      result = arms[i];
    }
  }
  return result;
}

}  // namespace bandit
}  // namespace core
}  // namespace jubatus
