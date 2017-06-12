// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2013 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include "jubatus/util/lang/cast.h"

#include "../anomaly/anomaly.hpp"
#include "../fv_converter/datum.hpp"
#include "../nearest_neighbor/nearest_neighbor.hpp"
#include "../recommender/recommender.hpp"
#include "../framework/stream_writer.hpp"
#include "../unlearner/unlearner.hpp"
#include "anomaly.hpp"
#include "test_util.hpp"

using std::make_pair;
using std::vector;
using std::string;
using std::pair;
using jubatus::util::lang::shared_ptr;
using jubatus::core::fv_converter::datum_to_fv_converter;
using jubatus::core::anomaly::anomaly_base;
using jubatus::util::lang::lexical_cast;

namespace jubatus {
namespace core {
namespace driver {

class anomaly_test
  : public ::testing::TestWithParam<shared_ptr<anomaly_base> > {
 protected:
  void SetUp() {
    anomaly_.reset(new driver::anomaly(
                     GetParam(), make_fv_converter()));
  }

  void TearDown() {
    anomaly_.reset();
  }

  jubatus::util::lang::shared_ptr<core::driver::anomaly> anomaly_;
};

TEST_P(anomaly_test, small) {
  {
    fv_converter::datum datum;
    datum.num_values_.push_back(make_pair("f1", 1.0));

    anomaly_->add("1", datum);  // is it good to be inf?
    pair<string, float> w = anomaly_->add("2", datum);
    float v = anomaly_->calc_score(datum);
    ASSERT_DOUBLE_EQ(w.second, v);
  }
  {
    vector<string> rows = anomaly_->get_all_rows();
    ASSERT_EQ(2u, rows.size());
  }


  // save
  msgpack::sbuffer sbuf;
  framework::stream_writer<msgpack::sbuffer> st(sbuf);
  framework::jubatus_packer jp(st);
  framework::packer pk(jp);
  anomaly_->pack(pk);

  // clear
  anomaly_->clear();
  {
    std::vector<std::string> rows = anomaly_->get_all_rows();
    ASSERT_EQ(0u, rows.size());
  }

  { // load
    msgpack::zone z;
    msgpack::object obj = msgpack::unpack(z, sbuf.data(), sbuf.size());
    anomaly_->unpack(obj);
    std::vector<std::string> rows = anomaly_->get_all_rows();
    ASSERT_EQ(2u, rows.size());
  }
}

vector<shared_ptr<anomaly_base> > create_anomaly_base() {
  vector<shared_ptr<anomaly_base> > method;

  const std::string ID;


  // lof
  core::anomaly::lof_storage::config lof_config;
  lof_config.nearest_neighbor_num = 10;
  lof_config.reverse_nearest_neighbor_num = 30;
  core::recommender::euclid_lsh::config lof_lsh_config;
  lof_lsh_config.hash_num = 8;
  lof_lsh_config.table_num = 8;
  lof_lsh_config.probe_num = 8;
  lof_lsh_config.bin_width = 8;
  lof_lsh_config.seed = 1234;

  method.push_back(shared_ptr<anomaly_base>(
      new core::anomaly::lof(lof_config,
          shared_ptr<core::recommender::recommender_base>(
              new core::recommender::euclid_lsh(lof_lsh_config)))));

  // light_lof
  core::anomaly::light_lof::config light_lof_config;
  light_lof_config.nearest_neighbor_num = 10;
  light_lof_config.reverse_nearest_neighbor_num = 30;
  core::nearest_neighbor::euclid_lsh::config light_lof_lsh_config;
  light_lof_lsh_config.hash_num = 8;

  shared_ptr<storage::column_table> nn_table(new storage::column_table);
  shared_ptr<core::nearest_neighbor::nearest_neighbor_base> nn_engine(
      new core::nearest_neighbor::euclid_lsh(
          light_lof_lsh_config, nn_table, ID));

  method.push_back(shared_ptr<anomaly_base>(
      new core::anomaly::light_lof(light_lof_config, ID, nn_engine)));

  return method;
}

INSTANTIATE_TEST_CASE_P(anomaly_test_instance,
    anomaly_test,
    testing::ValuesIn(create_anomaly_base()));


class light_lof_test : public ::testing::Test {
 protected:
  void SetUp() {
    core::anomaly::light_lof::config lof_config;
    lof_config.nearest_neighbor_num = 3;
    lof_config.reverse_nearest_neighbor_num = 3;

    shared_ptr<storage::column_table> lsh_table(
        new storage::column_table);
    core::nearest_neighbor::euclid_lsh::config lsh_config;
    shared_ptr<core::nearest_neighbor::nearest_neighbor_base> lsh(
        new core::nearest_neighbor::euclid_lsh(lsh_config, lsh_table, "id"));

    unlearner::lru_unlearner::config unlearner_config;
    unlearner_config.max_size = 5;
    jubatus::util::lang::shared_ptr<unlearner::unlearner_base> unlearner(
        new unlearner::lru_unlearner(unlearner_config));

    anomaly_.reset(new anomaly(
                       shared_ptr<anomaly_base>(
                           new core::anomaly::light_lof(
                               lof_config, "id", lsh, unlearner)),
                           make_fv_converter()));
  }

  jubatus::util::lang::shared_ptr<core::driver::anomaly> anomaly_;
};

TEST_F(light_lof_test, unlearning) {
  fv_converter::datum datum;
  datum.num_values_.push_back(make_pair("f1", 1.0));

  for (std::size_t i = 0; i < 5; ++i) {
    datum.num_values_[0].second = i;
    anomaly_->overwrite(
        "row" + jubatus::util::lang::lexical_cast<std::string>(i),
        datum);
    std::vector<std::string> rows = anomaly_->get_all_rows();
    EXPECT_EQ(i + 1, rows.size());
  }

  anomaly_->overwrite("0", datum);
  anomaly_->overwrite("10", datum);
  std::vector<std::string> rows = anomaly_->get_all_rows();
  EXPECT_EQ(5u, rows.size());
}

class anomaly_with_weight_manager_test
  : public ::testing::TestWithParam<shared_ptr<anomaly_base> > {
 protected:
  void SetUp() {
    anomaly_.reset(new driver::anomaly(
                     GetParam(), make_tf_idf_fv_converter()));
  }

  void TearDown() {
    anomaly_.reset();
  }
  jubatus::util::lang::shared_ptr<core::driver::anomaly> anomaly_;
};

fv_converter::datum create_datum_str(const string& key, const string& value) {
  fv_converter::datum d;
  d.string_values_.push_back(make_pair(key, value));
  return d;
}

TEST_P(anomaly_with_weight_manager_test, small) {
  std::pair<std::string, float> w = anomaly_->add(
      "a", create_datum_str("f2", "x y z"));
  fv_converter::datum datum = create_datum_str("f1", "a b c");
  for (int i = 0; i < 20; i += 2) {
    anomaly_->add(lexical_cast<string>(i), datum);  // is it good to be inf?
    std::pair<std::string, float> w =
        anomaly_->add(lexical_cast<string>(i + 1), datum);
    float v = anomaly_->calc_score(datum);
    ASSERT_GT(2.0, v);
  }

  std::vector<std::string> rows = anomaly_->get_all_rows();
  ASSERT_EQ(21U, rows.size());
}

INSTANTIATE_TEST_CASE_P(anomaly_with_weight_manager_test_instance,
    anomaly_with_weight_manager_test,
    testing::ValuesIn(create_anomaly_base()));
}  // driver namespace
}  // core namespace
}  // jubatus namespace
