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

#include "bandit.hpp"

#include <string>
#include <vector>
#include <gtest/gtest.h>

#include "jubatus/util/math/random.h"

using jubatus::util::lang::shared_ptr;
using jubatus::util::math::random::mtrand;
namespace json = jubatus::util::text::json;

namespace jubatus {
namespace core {
namespace driver {

double simulate_reward(const std::string& arm_id, mtrand& rand) {
  if (arm_id == "hoge") {
    if (rand.next_double() < 0.05) {
      return 1.0;
    } else {
      return 0.0;
    }
  } else if (arm_id == "fuga") {
    if (rand.next_double() < 0.15) {
      return 1.0;
    } else {
      return 0.0;
    }
  } else {
    if (rand.next_double() < 0.1) {
      return 1.0;
    } else {
      return 0.0;
    }
  }
}

TEST(bandit, epsilon_greedy) {
  json::json js(new json::json_object);
  js["assume_unrewarded"] = json::to_json(true);
  js["epsilon"] = json::to_json(0.1);
  common::jsonconfig::config conf(js);

  bandit b("epsilon_greedy", conf);

  b.register_arm("hoge");
  b.register_arm("fuga");
  b.register_arm("piyo");

  std::string player_id = "player";
  int trial = 10000;
  mtrand rand;
  double total_reward = 0;
  for (int i = 0; i < trial; ++i) {
    std::string arm = b.select_arm(player_id);

    double reward = simulate_reward(arm, rand);
    b.register_reward(player_id, arm, reward);
    total_reward += reward;
  }

  EXPECT_LT(0.1 * trial, total_reward);

  core::bandit::arm_info_map r = b.get_arm_info(player_id);
  int trial_actual = 0;
  double total_reward_actual = 0;
  for (core::bandit::arm_info_map::const_iterator iter = r.begin();
       iter != r.end(); ++iter) {
    trial_actual += iter->second.trial_count;
    total_reward_actual += iter->second.weight;
  }

  EXPECT_EQ(trial, trial_actual);
  EXPECT_DOUBLE_EQ(total_reward, total_reward_actual);
}

}  // driver namespace
}  // core namespace
}  // jubatus namespace
