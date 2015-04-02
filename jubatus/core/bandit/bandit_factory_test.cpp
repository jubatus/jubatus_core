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

#include "bandit_factory.hpp"

#include <string>

#include <gtest/gtest.h>
#include "../common/jsonconfig.hpp"
#include "bandit_base.hpp"

using jubatus::util::lang::shared_ptr;
namespace json = jubatus::util::text::json;

namespace jubatus {
namespace core {
namespace bandit {

TEST(bandit_factory, epsilon_greedy) {
  json::json js(new json::json_object);
  js["assume_unrewarded"] = json::to_json(true);
  js["epsilon"] = json::to_json(0.5);
  common::jsonconfig::config conf(js);
  shared_ptr<bandit_base> p = bandit_factory::create("epsilon_greedy", conf);
  EXPECT_EQ("epsilon_greedy", p->name());
}

TEST(bandit_factory, ucb1) {
  json::json js(new json::json_object);
  js["assume_unrewarded"] = json::to_json(true);
  common::jsonconfig::config conf(js);
  shared_ptr<bandit_base> p = bandit_factory::create("ucb1", conf);
  EXPECT_EQ("ucb1", p->name());
}

TEST(bandit_factory, softmax) {
  json::json js(new json::json_object);
  js["assume_unrewarded"] = json::to_json(true);
  js["tau"] = json::to_json(0.5);
  common::jsonconfig::config conf(js);
  shared_ptr<bandit_base> p = bandit_factory::create("softmax", conf);
  EXPECT_EQ("softmax", p->name());
}

TEST(bandit_factory, exp3) {
  json::json js(new json::json_object);
  js["assume_unrewarded"] = json::to_json(true);
  js["gamma"] = json::to_json(0.5);
  common::jsonconfig::config conf(js);
  shared_ptr<bandit_base> p = bandit_factory::create("exp3", conf);
  EXPECT_EQ("exp3", p->name());
}

}  // namespace bandit
}  // namespace core
}  // namespace jubatus
