// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2016 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#include <string>
#include <vector>
#include <utility>
#include <gtest/gtest.h>
#include "jubatus/util/lang/shared_ptr.h"

#include "inverted_index_euclid.hpp"
#include "../common/jsonconfig.hpp"
#include "../common/type.hpp"

using jubatus::util::text::json::json;
using jubatus::util::text::json::json_object;
using jubatus::util::text::json::to_json;
using jubatus::core::common::jsonconfig::config_cast;
using jubatus::core::common::sfv_t;

namespace jubatus {
namespace core {
namespace recommender {

TEST(inverted_index_euclid, config_validation_with_unlearner) {
  jubatus::util::lang::shared_ptr<inverted_index_euclid> r;

  {
    json js(new json_object);
    js["unlearner"] = to_json(std::string("lru"));
    js["unlearner_parameter"] = new json_object;
    js["unlearner_parameter"]["max_size"] = to_json(1);
    common::jsonconfig::config conf(js);
    ASSERT_NO_THROW(r.reset(new inverted_index_euclid(
        config_cast<inverted_index_euclid::config>(conf))));
  }

  {
    json js(new json_object);
    js["unlearner"] = to_json(std::string("lru"));
    common::jsonconfig::config conf(js);
    ASSERT_THROW(
      r.reset(new inverted_index_euclid(
          config_cast<inverted_index_euclid::config>(conf))),
      common::config_exception);
  }
}

TEST(inverted_index_euclid, config_validation_ignore_orthogonal) {
  jubatus::util::lang::shared_ptr<inverted_index_euclid> r;

  {
    json js(new json_object);
    {
      common::jsonconfig::config conf(js);
      ASSERT_NO_THROW(r.reset(new inverted_index_euclid(
          config_cast<inverted_index_euclid::config>(conf))));
    }
    {
      js["ignore_orthogonal"] = to_json(true);
      common::jsonconfig::config conf(js);
      ASSERT_NO_THROW(r.reset(new inverted_index_euclid(
          config_cast<inverted_index_euclid::config>(conf))));
    }
    {
      js["ignore_orthogonal"] = to_json(false);
      common::jsonconfig::config conf(js);
      ASSERT_NO_THROW(r.reset(new inverted_index_euclid(
          config_cast<inverted_index_euclid::config>(conf))));
    }
  }
}

TEST(inverted_index_euclid, trivial_test) {
  jubatus::util::lang::shared_ptr<inverted_index_euclid> r;
  json js(new json_object);
  common::jsonconfig::config conf(js);
  r.reset(new inverted_index_euclid(
      config_cast<inverted_index_euclid::config>(conf)));
  sfv_t v1, v2;
  std::vector<std::pair<std::string, double> > res;
  v1.push_back(std::pair<std::string, double>("a", 1.0));
  v1.push_back(std::pair<std::string, double>("b", 1.0));
  v2.push_back(std::pair<std::string, double>("a", 4.0));
  v2.push_back(std::pair<std::string, double>("b", 5.0));
  r->update_row("v1", v1);
  r->update_row("v2", v2);
  r->similar_row(v1, res, 2);
  EXPECT_EQ(res[0].first, "v1");
  EXPECT_EQ(res[0].second, 0);
  EXPECT_EQ(res[1].second, -5.0);
}

TEST(inverted_index_euclid, trivial_test_with_ignore_orthogonal) {
  jubatus::util::lang::shared_ptr<inverted_index_euclid> r;
  json js(new json_object);
  js["ignore_orthogonal"] = to_json(true);
  common::jsonconfig::config conf(js);
  r.reset(new inverted_index_euclid(
      config_cast<inverted_index_euclid::config>(conf)));
  sfv_t v1, v2, q1, q2, q3;

  std::vector<std::pair<std::string, double> > res;
  v1.push_back(std::pair<std::string, double>("a", 1.0));
  v1.push_back(std::pair<std::string, double>("b", 1.0));
  v2.push_back(std::pair<std::string, double>("a", 4.0));
  v2.push_back(std::pair<std::string, double>("c", 5.0));

  r->update_row("v1", v1);
  r->update_row("v2", v2);

  q1.push_back(std::pair<std::string, double>("a", 1.0));

  r->similar_row(q1, res, 2);

  EXPECT_EQ(res[0].first, "v1");
  EXPECT_EQ(res[0].second, -1.0);
  EXPECT_EQ(res[1].first, "v2");
  EXPECT_DOUBLE_EQ(res[1].second, -5.8309518948453);

  q2.push_back(std::pair<std::string, double>("c", 1.0));
  res.clear();
  r->similar_row(q2, res, 2);
  EXPECT_EQ(res.size(), 1u);
  EXPECT_EQ(res[0].first, "v2");
  EXPECT_DOUBLE_EQ(res[0].second, -5.65685424949238);

  q3.push_back(std::pair<std::string, double>("a", 1.0));
  q3.push_back(std::pair<std::string, double>("b", -1.0));
  res.clear();
  r->similar_row(q3, res, 2);
  EXPECT_EQ(res.size(), 2u);
  EXPECT_EQ(res[0].first, "v1");
  EXPECT_DOUBLE_EQ(res[0].second, -2.0);
}

}  // namespace recommender
}  // namespace core
}  // namespace jubatus
