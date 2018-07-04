// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2012 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#include "lof.hpp"

#include <string>
#include <utility>
#include <vector>
#include <sstream>
#include <gtest/gtest.h>
#include "jubatus/util/data/unordered_map.h"
#include "jubatus/util/text/json.h"
#include "../common/jsonconfig.hpp"
#include "../recommender/recommender.hpp"
#include "lof_storage.hpp"
#include "../unlearner/unlearner_base.hpp"
#include "../unlearner/unlearner_factory.hpp"

using jubatus::util::data::unordered_map;
using jubatus::util::lang::shared_ptr;
using jubatus::util::text::json::json;
using jubatus::util::text::json::json_object;
using jubatus::util::text::json::to_json;
using std::pair;
using std::string;
using std::vector;
using std::make_pair;

namespace jubatus {
namespace core {
namespace anomaly {

template<typename T>
class lof_test : public testing::Test {
};

TYPED_TEST_CASE_P(lof_test);

TYPED_TEST_P(lof_test, update_row) {
  lof l(lof_storage::config(), shared_ptr<TypeParam>(new TypeParam));
  common::sfv_t v, q;
  const string id = "test";
  l.update_row(id, v);
  l.calc_anomaly_score(id);
}

TYPED_TEST_P(lof_test, update_bulk) {
  lof l(lof_storage::config(), shared_ptr<TypeParam>(new TypeParam));
  vector<pair<string, common::sfv_t> > data;
  vector<string> ids;
  for (int i = 0; i < 10; i++) {
    common::sfv_t v;
    std::ostringstream id;
    id << i;
    v.push_back(make_pair("x", static_cast<double>(i)));
    data.push_back(make_pair(id.str(), v));
  }
  ids = l.update_bulk(data);
  EXPECT_EQ(ids.size(), 10);
}

TYPED_TEST_P(lof_test, bulk_ignore_kth) {
  lof_storage::config c;
  c.ignore_kth_same_point = jubatus::util::data::optional<bool>(true);
  lof l(c, shared_ptr<TypeParam>(new TypeParam));
  vector<pair<string, common::sfv_t> > data;
  vector<string> ids;
  common::sfv_t v;
  for (int i = 0; i < 10; i++) {
    v.clear();
    std::ostringstream id;
    id << i;
    v.push_back(make_pair("x", 1.0));
    v.push_back(make_pair("y", 1.0));
    data.push_back(make_pair(id.str(), v));
  }
  ids = l.update_bulk(data);
  vector<string> all_ids;
  l.get_all_row_ids(all_ids);
  EXPECT_EQ(ids.size(), 9);
  EXPECT_EQ(all_ids.size(), 9);
  EXPECT_EQ(l.calc_anomaly_score("0"), 0.0);
  v.clear();
  v.push_back(make_pair("x", 1.0));
  v.push_back(make_pair("y", -1.0));
  EXPECT_TRUE(l.update_row("10", v));
}

TYPED_TEST_P(lof_test, bulk_unlearner) {
  lof_storage::config c;
  c.ignore_kth_same_point = jubatus::util::data::optional<bool>(true);
  json js(new json_object);
  js["max_size"] = to_json(5);
  jubatus::core::common::jsonconfig::config conf(js);
  shared_ptr<jubatus::core::unlearner::unlearner_base> unlearner =
    jubatus::core::unlearner::create_unlearner(
                                               "lru",
                                               conf);
  lof l(c, shared_ptr<TypeParam>(new TypeParam), unlearner);
  vector<pair<string, common::sfv_t> > data;
  vector<string> ids;
  common::sfv_t v;
  for (int i = 0; i < 10; i++) {
    v.clear();
    std::ostringstream id;
    id << i;
    v.push_back(make_pair("x", 1.0));
    v.push_back(make_pair("y", 1.0));
    data.push_back(make_pair(id.str(), v));
  }
  ids = l.update_bulk(data);
  vector<string> all_ids;
  l.get_all_row_ids(all_ids);
  EXPECT_EQ(ids.size(), 10);  // update bulk will succeed for all points
  EXPECT_EQ(all_ids.size(), 5);  // the model has max_size points
  EXPECT_EQ(l.calc_anomaly_score("9"), 0.0);  // the last point's score will 0
}


TYPED_TEST_P(lof_test, set_row) {
  lof l(lof_storage::config(), shared_ptr<TypeParam>(new TypeParam));
  common::sfv_t v, q;
  const string id = "test";
  l.set_row(id, v);
  l.calc_anomaly_score(id);
}

TYPED_TEST_P(lof_test, config_validation) {
  shared_ptr<TypeParam> nn_engine(new TypeParam);
  shared_ptr<lof> l;
  lof_storage::config c;

  c.reverse_nearest_neighbor_num = 10;

  // nearest_neighbor_num <= 2
  c.nearest_neighbor_num = 1;
  ASSERT_THROW(l.reset(new lof(c, nn_engine)), common::invalid_parameter);

  c.nearest_neighbor_num = 2;
  ASSERT_NO_THROW(l.reset(new lof(c, nn_engine)));

  c.nearest_neighbor_num = 3;
  ASSERT_NO_THROW(l.reset(new lof(c, nn_engine)));

  // nearest_neighbor_num <= reverse_nearest_neighbor_num
  c.reverse_nearest_neighbor_num = 2;
  ASSERT_THROW(l.reset(new lof(c, nn_engine)), common::invalid_parameter);

  c.reverse_nearest_neighbor_num = 3;
  ASSERT_NO_THROW(l.reset(new lof(c, nn_engine)));
}

REGISTER_TYPED_TEST_CASE_P(lof_test,
                           update_row,
                           set_row,
                           config_validation,
                           update_bulk,
                           bulk_ignore_kth,
                           bulk_unlearner);

typedef testing::Types<recommender::inverted_index, recommender::lsh,
  recommender::minhash, recommender::euclid_lsh> recommender_types;

INSTANTIATE_TYPED_TEST_CASE_P(lt,
  lof_test, recommender_types);

}  // namespace anomaly
}  // namespace core
}  // namespace jubatus
