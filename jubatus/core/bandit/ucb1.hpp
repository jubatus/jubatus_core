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

#ifndef JUBATUS_CORE_BANDIT_UCB1_HPP_
#define JUBATUS_CORE_BANDIT_UCB1_HPP_

#include <string>

#include "bandit_base.hpp"

namespace jubatus {
namespace core {
namespace bandit {

class ucb1 : public bandit_base {
 public:
  explicit ucb1(const jubatus::util::lang::shared_ptr<storage>& s);

  std::string select_arm(const std::string& player_id);

  std::string name() const {
    return "ucb1";
  }
};

}  // namespace bandit
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_BANDIT_UCB1_HPP_
