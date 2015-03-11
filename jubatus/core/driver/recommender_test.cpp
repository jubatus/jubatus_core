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

#include <vector>
#include <utility>
#include <map>
#include <string>

#include <gtest/gtest.h>

#include "jubatus/util/concurrent/thread.h"
#include "jubatus/util/lang/cast.h"
#include "jubatus/util/lang/shared_ptr.h"
#include "jubatus/util/lang/function.h"
#include "jubatus/util/lang/bind.h"
#include "jubatus/util/math/random.h"
#include "../common/jsonconfig.hpp"

#include "../fv_converter/datum.hpp"
#include "../fv_converter/converter_config.hpp"
#include "../recommender/recommender.hpp"
#include "../recommender/recommender_type.hpp"
#include "../recommender/recommender_factory.hpp"
#include "../classifier/classifier_test_util.hpp"
#include "../framework/stream_writer.hpp"
#include "../storage/column_table.hpp"
#include "recommender.hpp"

#include "test_util.hpp"

using std::string;
using std::map;
using std::vector;
using std::pair;
using std::make_pair;
using std::stringstream;
using jubatus::util::lang::shared_ptr;
using jubatus::util::text::json::json;
using jubatus::util::text::json::json_object;
using jubatus::util::text::json::json_integer;
using jubatus::util::text::json::json_string;
using jubatus::util::text::json::json_float;
using jubatus::util::lang::lexical_cast;
using jubatus::util::concurrent::thread;
using jubatus::util::lang::function;
using jubatus::util::lang::bind;
using jubatus::util::lang::_1;
using jubatus::util::lang::_2;
using jubatus::core::fv_converter::datum;
using jubatus::core::recommender::recommender_base;
using jubatus::core::storage::column_table;
namespace jubatus {
namespace core {
namespace driver {

class recommender_test : public ::testing::Test {
 protected:
  void SetUp() {
    recommender_.reset(new driver::recommender(
          jubatus::util::lang::shared_ptr<core::recommender::recommender_base>(
            new core::recommender::inverted_index),
          make_fv_converter()));
  }

  void TearDown() {
    recommender_.reset();
  }

  jubatus::util::lang::shared_ptr<core::driver::recommender> recommender_;
};


TEST_F(recommender_test, small) {
  datum d;
  d.num_values_.push_back(make_pair("f1", 1.0));
  recommender_->update_row("key", d);
  recommender_->clear_row("key");
  recommender_->update_row("key", d);

  recommender_->complete_row_from_datum(d);
  recommender_->complete_row_from_id("key");
}

class nn_recommender_test
    : public ::testing::TestWithParam<
        shared_ptr<core::recommender::recommender_base> > {
 protected:
  void SetUp() {
    recommender_.reset(new driver::recommender(
          jubatus::util::lang::shared_ptr<core::recommender::recommender_base>(
            new core::recommender::inverted_index),
          make_tf_idf_fv_converter()));
  }

  void TearDown() {
    recommender_.reset();
  }

  jubatus::util::lang::shared_ptr<core::driver::recommender> recommender_;
};

vector<shared_ptr<recommender_base> >
create_recommender_bases() {
  const std::string id("my_id");
  vector<shared_ptr<recommender_base> > recommenders;

  vector<pair<string, int> > pattern;
  for (size_t i = 8; i < 3000; i = i << 1) {  // up to 2048
    pattern.push_back(make_pair("lsh", i));
    pattern.push_back(make_pair("euclid_lsh", i));
    pattern.push_back(make_pair("minhash", i));
  }
  for (size_t i = 0; i < pattern.size(); ++i) {
    shared_ptr<column_table> table(new column_table);

    json jsconf(new json_object);
    json method_param(new json_object);
    method_param["hash_num"] = new json_integer(pattern[i].second);
    jsconf["parameter"] = method_param;
    jsconf["method"] = new json_string(pattern[i].first);
    common::jsonconfig::config conf(jsconf);
    recommenders.push_back(
        core::recommender::recommender_factory::create_recommender(
            "nearest_neighbor_recommender",
            conf,
            id));
  }
  return recommenders;
}

fv_converter::datum create_datum_str(const string& key, const string& value) {
  fv_converter::datum d;
  d.string_values_.push_back(make_pair(key, value));
  return d;
}

TEST_P(nn_recommender_test, update) {
  datum d = create_datum_str("a", "f g h");
  for (int i = 0; i < 10; ++i) {
    recommender_->update_row("id1", create_datum_str("a", "a b c"));
    recommender_->update_row("id2", create_datum_str("a", "d e f"));
    recommender_->update_row("id3", create_datum_str("a", "e f g"));
    recommender_->update_row("id4", create_datum_str("a", "f g h"));
    recommender_->update_row("id5", create_datum_str("a", "h i j"));
    recommender_->update_row("id6", create_datum_str("a", "i j a"));
    recommender_->update_row("id7", create_datum_str("a", "j a b"));
  }

  vector<pair<string, float> > ret =
      recommender_->similar_row_from_datum(d, 10);
  ASSERT_EQ("id4", ret[0].first);
}

INSTANTIATE_TEST_CASE_P(nn_recommender_test_instance,
    nn_recommender_test,
    testing::ValuesIn(create_recommender_bases()));

datum make_datum(const string& key, const string& vec) {
  datum d;
  d.string_values_.push_back(make_pair(key, vec));
  return d;
}

void make_random_data(
    jubatus::util::math::random::mtrand& rand,
    vector<pair<string, datum> >& data,
    size_t size) {
  for (size_t i = 0; i < size; ++i) {
    string payload;
    for (size_t j = 0; j < 20; ++j) {
      payload += lexical_cast<string>(rand() % 100) + " ";
    }
    data.push_back(make_pair("key", make_datum("str", payload)));
  }
}

class concurrent_recommender_test
    : public ::testing::TestWithParam<
        shared_ptr<core::recommender::recommender_base> > {
 protected:
  void SetUp() {
    recommender_.reset(new driver::recommender(
          jubatus::util::lang::shared_ptr<core::recommender::recommender_base>(
            new core::recommender::inverted_index),
          make_tf_idf_fv_converter()));
  }

  void TearDown() {
    recommender_.reset();
  }

  jubatus::util::lang::shared_ptr<core::driver::recommender> recommender_;
};

vector<shared_ptr<recommender_base> >
create_recommender_bases_subset() {
  const std::string id("my_id");
  vector<shared_ptr<recommender_base> > recommenders;

  {
    common::jsonconfig::config conf;
    recommenders.push_back(
        core::recommender::recommender_factory::create_recommender(
            "inverted_index",
            conf,
            ""));
  }

  vector<pair<string, int> > pattern;
  pattern.push_back(make_pair("lsh", 2048));
  pattern.push_back(make_pair("euclid_lsh", 2048));
  pattern.push_back(make_pair("minhash", 2048));

  for (size_t i = 0; i < pattern.size(); ++i) {
    shared_ptr<core::table::column_table> table(new core::table::column_table);

    json jsconf(new json_object);
    json method_param(new json_object);
    method_param["hash_num"] = new json_integer(pattern[i].second);
    jsconf["parameter"] = method_param;
    jsconf["method"] = new json_string(pattern[i].first);
    common::jsonconfig::config conf(jsconf);
    recommenders.push_back(
        core::recommender::recommender_factory::create_recommender(
            "nearest_neighbor_recommender",
            conf,
            id));
  }

  return recommenders;
}

void update_row_task(int thread_id,
                     int num,
                     shared_ptr<driver::recommender> target) {
  jubatus::util::math::random::mtrand rand;
  vector<pair<string, datum> > data;
  make_random_data(rand, data, num);
  for (int i = 0; i < num; ++i) {
    target->update_row(lexical_cast<string>(thread_id) +
                       "|" +
                       lexical_cast<string>(num),
                       data[i].second);
  }
}

void clear_row_task(int thread_id,
                     int num,
                     shared_ptr<driver::recommender> target) {
  jubatus::util::math::random::mtrand rand;
  for (int i = 0; i < num; ++i) {
    target->clear_row(lexical_cast<string>(thread_id) +
                       "|" +
                       lexical_cast<string>(num));
  }
}

void similar_row_from_datum_task(int thread_id,
                                 int num,
                                 shared_ptr<driver::recommender> target) {
  jubatus::util::math::random::mtrand rand;
  vector<pair<string, datum> > data;
  make_random_data(rand, data, num);

  for (int i = 0; i < num; ++i) {
    target->similar_row_from_datum(data[i].second, 10);
  }
}

TEST_P(concurrent_recommender_test, update_update) {
  thread t0(bind(update_row_task, 0, 100, recommender_));
  thread t1(bind(update_row_task, 1, 100, recommender_));
  t0.start();
  t1.start();
  t0.join();
  t1.join();
}

TEST_P(concurrent_recommender_test, update_clear) {
  thread t0(bind(update_row_task, 0, 200, recommender_));
  thread t1(bind(clear_row_task, 0, 200, recommender_));  // on purpose
  t0.start();
  t1.start();
  t1.sleep(0.001);  // delete start a bit later
  t0.join();
  t1.join();
}

TEST_P(concurrent_recommender_test, update_clear_similar) {
  thread t0(bind(update_row_task, 0, 200, recommender_));
  thread t1(bind(clear_row_task, 0, 200, recommender_));  // on purpose
  thread t2(bind(similar_row_from_datum_task, 1, 200, recommender_));
  t0.start();
  t1.start();
  t2.start();
  t1.sleep(0.001);
  t0.join();
  t1.join();
  t2.join();
}


INSTANTIATE_TEST_CASE_P(concurrent_recommender_test_instance,
                        concurrent_recommender_test,
                        testing::ValuesIn(create_recommender_bases_subset()));


}  // namespace driver
}  // namespace core
}  // namespace jubatus
