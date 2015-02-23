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

#include <gtest/gtest.h>

namespace jubatus {
namespace core {
namespace bandit {

TEST(summation_storage, common) {
  summation_storage s;
  const std::string player_id = "player1";
  const std::string arm_id = "arm1";

  {
    arm_info a = s.get_arm_info(player_id, arm_id);
    EXPECT_EQ(0, a.trial_count);
    EXPECT_EQ(0.0, a.weight);
    EXPECT_EQ(0.0, s.get_expectation(player_id, arm_id));
  }

  s.register_reward(player_id, arm_id, 1.0);

  {
    arm_info a = s.get_arm_info(player_id, arm_id);
    EXPECT_EQ(1, a.trial_count);
    EXPECT_EQ(1.0, a.weight);
    EXPECT_EQ(1.0, s.get_expectation(player_id, arm_id));
  }

  s.register_reward(player_id, arm_id, 0.0);

  {
    arm_info a = s.get_arm_info(player_id, arm_id);
    EXPECT_EQ(2, a.trial_count);
    EXPECT_EQ(1.0, a.weight);
    EXPECT_EQ(0.5, s.get_expectation(player_id, arm_id));
  }

  s.clear();

  {
    arm_info a = s.get_arm_info(player_id, arm_id);
    EXPECT_EQ(0, a.trial_count);
    EXPECT_EQ(0.0, a.weight);
    EXPECT_EQ(0.0, s.get_expectation(player_id, arm_id));
  }
}

TEST(storage, mix) {
  summation_storage s1, s2;

  s1.register_reward("player1", "hoge", 1.0);
  s1.register_reward("player1", "fuga", 0.0);
  s1.register_reward("player2", "hoge", 0.0);
  s2.register_reward("player1", "hoge", 0.0);
  s2.register_reward("player1", "piyo", 1.0);
  s2.register_reward("player2", "hoge", 0.5);

  summation_storage::table_t diff1, diff2;
  s1.get_diff(diff1);
  s2.get_diff(diff2);
  summation_storage::mix(diff2, diff1);
  s1.put_diff(diff1);
  s2.put_diff(diff1);

  {
    arm_info a = s1.get_arm_info("player1", "hoge");
    EXPECT_EQ(2, a.trial_count);
    EXPECT_EQ(1.0, a.weight);
    EXPECT_EQ(0.5, s1.get_expectation("player1", "hoge"));
  }
  {
    arm_info a = s2.get_arm_info("player1", "hoge");
    EXPECT_EQ(2, a.trial_count);
    EXPECT_EQ(1.0, a.weight);
    EXPECT_EQ(0.5, s2.get_expectation("player1", "hoge"));
  }

  {
    arm_info a = s1.get_arm_info("player1", "fuga");
    EXPECT_EQ(1, a.trial_count);
    EXPECT_EQ(0.0, a.weight);
    EXPECT_EQ(0.0, s1.get_expectation("player1", "fuga"));
  }
  {
    arm_info a = s2.get_arm_info("player1", "fuga");
    EXPECT_EQ(1, a.trial_count);
    EXPECT_EQ(0.0, a.weight);
    EXPECT_EQ(0.0, s2.get_expectation("player1", "fuga"));
  }

  {
    arm_info a = s1.get_arm_info("player1", "piyo");
    EXPECT_EQ(1, a.trial_count);
    EXPECT_EQ(1.0, a.weight);
    EXPECT_EQ(1.0, s1.get_expectation("player1", "piyo"));
  }
  {
    arm_info a = s2.get_arm_info("player1", "piyo");
    EXPECT_EQ(1, a.trial_count);
    EXPECT_EQ(1.0, a.weight);
    EXPECT_EQ(1.0, s2.get_expectation("player1", "piyo"));
  }

  {
    arm_info a = s1.get_arm_info("player2", "hoge");
    EXPECT_EQ(2, a.trial_count);
    EXPECT_EQ(0.5, a.weight);
    EXPECT_EQ(0.25, s1.get_expectation("player2", "hoge"));
  }
  {
    arm_info a = s2.get_arm_info("player2", "hoge");
    EXPECT_EQ(2, a.trial_count);
    EXPECT_EQ(0.5, a.weight);
    EXPECT_EQ(0.25, s2.get_expectation("player2", "hoge"));
  }

  s1.register_reward("player1", "hoge", 1.0);

  {
    arm_info a = s1.get_arm_info("player1", "hoge");
    EXPECT_EQ(3, a.trial_count);
    EXPECT_EQ(2.0, a.weight);
    EXPECT_DOUBLE_EQ(2.0/3.0, s1.get_expectation("player1", "hoge"));
  }
  {
    arm_info a = s2.get_arm_info("player1", "hoge");
    EXPECT_EQ(2, a.trial_count);
    EXPECT_EQ(1.0, a.weight);
    EXPECT_EQ(0.5, s2.get_expectation("player1", "hoge"));
  }
}

}  // namespace bandit
}  // namespace core
}  // namespace jubatus
