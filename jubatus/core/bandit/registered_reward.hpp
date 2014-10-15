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

#ifndef JUBATUS_CORE_BANDIT_REGISTERED_REWARD_HPP_
#define JUBATUS_CORE_BANDIT_REGISTERED_REWARD_HPP_

#include <string>
#include <msgpack.hpp>

#include "jubatus/util/data/unordered_map.h"

namespace jubatus {
namespace core {
namespace bandit {

struct registered_reward {
  int trial_count;
  double total_reward;

  MSGPACK_DEFINE(trial_count, total_reward);
};

typedef jubatus::util::data::unordered_map<std::string, registered_reward>
    registered_rewards;

}  // namespace bandit
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_BANDIT_REGISTERED_REWARD_HPP_
