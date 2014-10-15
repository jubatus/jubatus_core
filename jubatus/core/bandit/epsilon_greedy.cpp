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

#include "epsilon_greedy.hpp"

#include <string>
#include <vector>
#include "../common/exception.hpp"

namespace jubatus {
namespace core {
namespace bandit {

epsilon_greedy::epsilon_greedy(
    const jubatus::util::lang::shared_ptr<storage>& s,
    double eps)
    : bandit_base(s), eps_(eps) {
  if (eps < 0 || 1 < eps) {
    throw JUBATUS_EXCEPTION(
        common::invalid_parameter("0 <= epsilon <= 1"));
  }
}

std::string epsilon_greedy::select_arm(const std::string& player_id) {
  const std::vector<std::string>& arms = get_arms();
  if (arms.empty()) {
    throw JUBATUS_EXCEPTION(
        common::exception::runtime_error("arm is not registered"));
  }

  if (rand_.next_double() < eps_) {
    // exploration
    return arms[rand_.next_int(arms.size())];
  } else {
    // exploitation
    const storage& s = get_storage();
    std::string result = arms[0];
    double exp_max = s.get_expectation(player_id, arms[0]);
    for (size_t i = 1; i < arms.size(); ++i) {
      double exp = s.get_expectation(player_id, arms[i]);
      if (exp > exp_max) {
        result = arms[i];
        exp_max = exp;
      }
    }
    return result;
  }
}

}  // namespace bandit
}  // namespace core
}  // namespace jubatus
