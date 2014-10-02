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

#include <gtest/gtest.h>

namespace jubatus {
namespace core {
namespace bandit {

TEST(storage, common) {
  storage s;
  const std::string player_id = "player1";
  const std::string arm_id = "arm1";

  {
    registered_reward r = s.get_registered_reward(player_id, arm_id);
    EXPECT_EQ(0, r.trial_count);
    EXPECT_EQ(0.0, r.total_reward);
    EXPECT_EQ(0.0, s.get_expectation(player_id, arm_id));
  }

  s.register_reward(player_id, arm_id, 1.0);

  {
    registered_reward r = s.get_registered_reward(player_id, arm_id);
    EXPECT_EQ(1, r.trial_count);
    EXPECT_EQ(1.0, r.total_reward);
    EXPECT_EQ(1.0, s.get_expectation(player_id, arm_id));
  }

  s.register_reward(player_id, arm_id, 0.0);

  {
    registered_reward r = s.get_registered_reward(player_id, arm_id);
    EXPECT_EQ(2, r.trial_count);
    EXPECT_EQ(1.0, r.total_reward);
    EXPECT_EQ(0.5, s.get_expectation(player_id, arm_id));
  }

  s.clear();

  {
    registered_reward r = s.get_registered_reward(player_id, arm_id);
    EXPECT_EQ(0, r.trial_count);
    EXPECT_EQ(0.0, r.total_reward);
    EXPECT_EQ(0.0, s.get_expectation(player_id, arm_id));
  }
}

TEST(storage, mix) {
  storage s1, s2;

  s1.register_reward("player1", "hoge", 1.0);
  s1.register_reward("player1", "fuga", 0.0);
  s1.register_reward("player2", "hoge", 0.0);
  s2.register_reward("player1", "hoge", 0.0);
  s2.register_reward("player1", "piyo", 1.0);
  s2.register_reward("player2", "hoge", 0.5);

  storage::table_t diff1, diff2;
  s1.get_diff(diff1);
  s2.get_diff(diff2);
  storage::mix(diff2, diff1);
  s1.put_diff(diff1);
  s2.put_diff(diff1);

  {
    registered_reward r = s1.get_registered_reward("player1", "hoge");
    EXPECT_EQ(2, r.trial_count);
    EXPECT_EQ(1.0, r.total_reward);
    EXPECT_EQ(0.5, s1.get_expectation("player1", "hoge"));
  }
  {
    registered_reward r = s2.get_registered_reward("player1", "hoge");
    EXPECT_EQ(2, r.trial_count);
    EXPECT_EQ(1.0, r.total_reward);
    EXPECT_EQ(0.5, s2.get_expectation("player1", "hoge"));
  }

  {
    registered_reward r = s1.get_registered_reward("player1", "fuga");
    EXPECT_EQ(1, r.trial_count);
    EXPECT_EQ(0.0, r.total_reward);
    EXPECT_EQ(0.0, s1.get_expectation("player1", "fuga"));
  }
  {
    registered_reward r = s2.get_registered_reward("player1", "fuga");
    EXPECT_EQ(1, r.trial_count);
    EXPECT_EQ(0.0, r.total_reward);
    EXPECT_EQ(0.0, s2.get_expectation("player1", "fuga"));
  }

  {
    registered_reward r = s1.get_registered_reward("player1", "piyo");
    EXPECT_EQ(1, r.trial_count);
    EXPECT_EQ(1.0, r.total_reward);
    EXPECT_EQ(1.0, s1.get_expectation("player1", "piyo"));
  }
  {
    registered_reward r = s2.get_registered_reward("player1", "piyo");
    EXPECT_EQ(1, r.trial_count);
    EXPECT_EQ(1.0, r.total_reward);
    EXPECT_EQ(1.0, s2.get_expectation("player1", "piyo"));
  }

  {
    registered_reward r = s1.get_registered_reward("player2", "hoge");
    EXPECT_EQ(2, r.trial_count);
    EXPECT_EQ(0.5, r.total_reward);
    EXPECT_EQ(0.25, s1.get_expectation("player2", "hoge"));
  }
  {
    registered_reward r = s2.get_registered_reward("player2", "hoge");
    EXPECT_EQ(2, r.trial_count);
    EXPECT_EQ(0.5, r.total_reward);
    EXPECT_EQ(0.25, s2.get_expectation("player2", "hoge"));
  }

  s1.register_reward("player1", "hoge", 1.0);

  {
    registered_reward r = s1.get_registered_reward("player1", "hoge");
    EXPECT_EQ(3, r.trial_count);
    EXPECT_EQ(2.0, r.total_reward);
    EXPECT_DOUBLE_EQ(2.0/3.0, s1.get_expectation("player1", "hoge"));
  }
  {
    registered_reward r = s2.get_registered_reward("player1", "hoge");
    EXPECT_EQ(2, r.trial_count);
    EXPECT_EQ(1.0, r.total_reward);
    EXPECT_EQ(0.5, s2.get_expectation("player1", "hoge"));
  }
}

}  // namespace bandit
}  // namespace core
}  // namespace jubatus
