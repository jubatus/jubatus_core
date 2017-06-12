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

#include <limits>
#include <string>
#include <utility>
#include <vector>
#include <gtest/gtest.h>

#include "../storage/local_storage.hpp"
#include "../regression/regression.hpp"
#include "../regression/regression_test_util.hpp"
#include "../fv_converter/datum.hpp"
#include "../fv_converter/converter_config.hpp"
#include "../framework/stream_writer.hpp"
#include "../common/jsonconfig.hpp"
#include "regression.hpp"
#include "jubatus/util/text/json.h"
#include "../storage/column_table.hpp"
#include "../nearest_neighbor/nearest_neighbor_factory.hpp"

#include "test_util.hpp"

using std::vector;
using std::pair;
using std::make_pair;
using std::string;
using std::cout;
using std::endl;
using jubatus::util::lang::lexical_cast;
using jubatus::util::lang::shared_ptr;
using jubatus::core::fv_converter::datum;
using jubatus::util::text::json::json;
using jubatus::util::text::json::to_json;
using jubatus::util::text::json::json_object;
using jubatus::core::common::jsonconfig::config_cast_check;

namespace jubatus {
namespace core {
namespace driver {

class regression_test : public ::testing::Test {
 protected:
  void SetUp() {
    shared_ptr<storage::storage_base> storage(new storage::local_storage);
    core::regression::passive_aggressive_1::config config;
    config.regularization_weight = std::numeric_limits<float>::max();
    config.sensitivity = 0.1f;
    shared_ptr<core::regression::regression_base> method(
        new core::regression::passive_aggressive_1(config, storage));
    regression_.reset(
      new core::driver::regression(
        method,
        make_fv_converter()));
  }

  void TearDown() {
    regression_.reset();
  }

  void my_test();

  jubatus::util::lang::shared_ptr<core::driver::regression> regression_;
};

datum convert_vector(const vector<double>& vec) {
  datum d;
  for (size_t i = 0; i < vec.size(); i++) {
    string f = "f" + lexical_cast<string>(i);
    d.num_values_.push_back(make_pair(f, vec[i]));
  }
  return d;
}

void make_random_data(vector<pair<float, datum> >& data, size_t size) {
  for (size_t i = 0; i < size; ++i) {
    pair<float, vector<double> > p = gen_random_data(1.0, 1.0, 10);
    data.push_back(make_pair(p.first, convert_vector(p.second)));
  }
}

shared_ptr<core::regression::regression_base> make_nn_regression() {
  json js(new json_object);
  js["method"] = to_json(std::string("lsh"));
  js["parameter"] = json(new json_object);
  js["parameter"]["hash_num"] = to_json(8);
  js["nearest_neighbor_num"] = to_json(5);
  common::jsonconfig::config param(js);
  core::regression::nearest_neighbor_regression::config conf
    = config_cast_check<core::regression::nearest_neighbor_regression::config>(
        param);
  shared_ptr<core::storage::column_table> table(new storage::column_table);
  shared_ptr<core::nearest_neighbor::nearest_neighbor_base>
    nearest_neighbor_engine(nearest_neighbor::create_nearest_neighbor(
       conf.method, conf.parameter, table, ""));
  shared_ptr<core::regression::regression_base> res(
       new core::regression::nearest_neighbor_regression(
           nearest_neighbor_engine,
           conf.nearest_neighbor_num));
  return res;
}

shared_ptr<core::regression::regression_base> make_inverted_index_regression() {
  shared_ptr<storage::storage_base> storage(new storage::local_storage);
  return shared_ptr<core::regression::regression_base> (
      new core::regression::cosine_similarity_regression(10));
}

void regression_test::my_test() {
  const size_t example_size = 1000;

  vector<pair<float, datum> > data;
  make_random_data(data, example_size);
  for (size_t i = 0; i < example_size; i++) {
    regression_->train(data[i]);
  }

  vector<float> values, result;
  {
    vector<pair<float, datum> >::const_iterator it;
    for (it = data.begin(); it != data.end(); ++it) {
      values.push_back(it->first);
      result.push_back(regression_->estimate(it->second));
    }
  }

  ASSERT_EQ(example_size, result.size());
  ASSERT_EQ(data.size(), result.size());

  vector<float>::const_iterator it0;  // answers
  vector<float>::const_iterator it;
  size_t count = 0;
  for (it = result.begin(), it0 = values.begin();
      it != result.end() && it0 != values.end(); ++it, ++it0) {
    if (std::fabs(*it0 - *it) < 2.0) {
      count++;
    }
  }

  // num of wrong classification should be less than 1%
  EXPECT_GE(count, result.size() - 10);
}

TEST_F(regression_test, pa) {
  my_test();
}

TEST_F(regression_test, small) {
  cout << "train" << endl;
  datum d;
  d.num_values_.push_back(make_pair("f1", 1.0));
  pair<float, datum> data(10, d);
  regression_->train(data);
  // save
  msgpack::sbuffer save_data;
  framework::stream_writer<msgpack::sbuffer> st(save_data);
  framework::jubatus_packer jp(st);
  framework::packer save_pk(jp);
  regression_->pack(save_pk);

  regression_->clear();

  // load
  msgpack::zone z;
  msgpack::object o = msgpack::unpack(z, save_data.data(), save_data.size());
  regression_->unpack(o);

  cout << "estimate" << endl;
  float res = regression_->estimate(d);
  cout << res << endl;
}

TEST_F(regression_test, nn) {
  regression_.reset(
      new core::driver::regression(
        make_nn_regression(),
        make_fv_converter()));

  datum d;
  d.num_values_.push_back(make_pair("f1", 1.0));
  pair<float, datum> data(10, d);
  regression_->train(data);

  // save
  msgpack::sbuffer save_data;
  framework::stream_writer<msgpack::sbuffer> st(save_data);
  framework::jubatus_packer jp(st);
  framework::packer save_pk(jp);
  regression_->pack(save_pk);

  regression_->clear();

  // load
  msgpack::zone z;
  msgpack::object o = msgpack::unpack(z, save_data.data(), save_data.size());
  regression_->unpack(o);

  cout << "estimate" << endl;
  float res = regression_->estimate(d);
  cout << res << endl;
}

TEST_F(regression_test, inverted_index) {
  regression_.reset(
      new core::driver::regression(
        make_inverted_index_regression(),
        make_fv_converter()));

  datum d;
  d.num_values_.push_back(make_pair("f1", 1.0));
  pair<float, datum> data(10, d);
  regression_->train(data);

  // save
  msgpack::sbuffer save_data;
  framework::stream_writer<msgpack::sbuffer> st(save_data);
  framework::jubatus_packer jp(st);
  framework::packer save_pk(jp);
  regression_->pack(save_pk);

  regression_->clear();

  // load
  msgpack::zone z;
  msgpack::object o = msgpack::unpack(z, save_data.data(), save_data.size());
  regression_->unpack(o);

  cout << "estimate" << endl;
  float res = regression_->estimate(d);
  cout << res << endl;
}

}  // namespace driver
}  // namespace core
}  // namespace jubatus
