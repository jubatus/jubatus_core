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

#include "summation_storage.hpp"

#include <string>
#include <gtest/gtest.h>
#include "../common/exception.hpp"

namespace jubatus {
namespace core {
namespace bandit {

TEST(summation_storage, common) {
  summation_storage s(false);
  const std::string player_id = "player1";
  const std::string arm_id = "arm1";
  s.register_arm(arm_id);

  {
    arm_info a = s.get_arm_info(player_id, arm_id);
    EXPECT_EQ(0, a.trial_count);
    EXPECT_EQ(0.0, a.weight);
    EXPECT_EQ(0.0, s.get_expectation(player_id, arm_id));
    EXPECT_EQ(0, s.get_total_trial_count(player_id));
  }

  s.register_reward(player_id, arm_id, 1.0);

  {
    arm_info a = s.get_arm_info(player_id, arm_id);
    EXPECT_EQ(1, a.trial_count);
    EXPECT_EQ(1.0, a.weight);
    EXPECT_EQ(1.0, s.get_expectation(player_id, arm_id));
    EXPECT_EQ(1, s.get_total_trial_count(player_id));
  }

  s.register_reward(player_id, arm_id, 0.0);

  {
    arm_info a = s.get_arm_info(player_id, arm_id);
    EXPECT_EQ(2, a.trial_count);
    EXPECT_EQ(1.0, a.weight);
    EXPECT_EQ(0.5, s.get_expectation(player_id, arm_id));
    EXPECT_EQ(2, s.get_total_trial_count(player_id));
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
  summation_storage s1(false), s2(false);
  s1.register_arm("hoge");
  s1.register_arm("fuga");
  s1.register_arm("piyo");
  s2.register_arm("hoge");
  s2.register_arm("fuga");
  s2.register_arm("piyo");

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
  {
    EXPECT_EQ(4, s1.get_total_trial_count("player1"));
    EXPECT_EQ(2, s1.get_total_trial_count("player2"));
    EXPECT_EQ(4, s2.get_total_trial_count("player1"));
    EXPECT_EQ(2, s2.get_total_trial_count("player2"));
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

TEST(summation_storage, notify_selected) {
  const std::string player_id = "player1";
  const std::string arm_id = "arm1";

  summation_storage s1(false);
  s1.register_arm(arm_id);

  s1.notify_selected(player_id, arm_id);

  // if assume_unrewarded is false, notify_selected has no effect
  {
    arm_info a = s1.get_arm_info(player_id, arm_id);
    EXPECT_EQ(0, a.trial_count);
    EXPECT_EQ(0.0, a.weight);
    EXPECT_EQ(0, s1.get_total_trial_count(player_id));
  }

  summation_storage s2(true);
  s2.register_arm(arm_id);

  s2.notify_selected(player_id, arm_id);

  // if assume_unrewarded is true, notify_selected will increment trial_count
  {
    arm_info a = s2.get_arm_info(player_id, arm_id);
    EXPECT_EQ(1, a.trial_count);
    EXPECT_EQ(0.0, a.weight);
    EXPECT_EQ(1, s2.get_total_trial_count(player_id));
  }

  s2.register_reward(player_id, arm_id, 1.0);

  // and register_reward will not increment trial_count
  {
    arm_info a = s2.get_arm_info(player_id, arm_id);
    EXPECT_EQ(1, a.trial_count);
    EXPECT_EQ(1.0, a.weight);
    EXPECT_EQ(1, s2.get_total_trial_count(player_id));
  }
}

TEST(summation_storage, unregistered_arm) {
  summation_storage s(false);
  EXPECT_THROW({
      s.register_reward("player1", "arm1", 0);
  }, common::exception::runtime_error);

  s.register_arm("arm1");
  EXPECT_NO_THROW({
      s.register_reward("player1", "arm1", 0);
  });
  EXPECT_NO_THROW({
      s.register_reward("player2", "arm1", 0);
  });

  EXPECT_THROW({
      s.register_reward("player1", "arm2", 0);
  }, common::exception::runtime_error);
  s.register_arm("arm2");
  EXPECT_NO_THROW({
      s.register_reward("player1", "arm2", 0);
  });
  EXPECT_NO_THROW({
      s.register_reward("player2", "arm2", 0);
  });
}

TEST(summation_storage, delete_arm){
  summation_storage s(false);
  const std::string player_id = "player1";
  EXPECT_FALSE(s.delete_arm("arm1"));
  s.register_arm("arm1");
  s.register_reward(player_id, "arm1", 1.0);
  EXPECT_EQ(1, s.get_total_trial_count(player_id));
  EXPECT_TRUE(s.delete_arm("arm1"));
  EXPECT_EQ(0, s.get_total_trial_count(player_id));
}
}  // namespace bandit
}  // namespace core
}  // namespace jubatus
