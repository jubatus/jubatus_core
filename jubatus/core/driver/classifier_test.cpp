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

#include <iostream>
#include <iterator>
#include <limits>
#include <string>
#include <utility>
#include <vector>
#include <map>

#include <gtest/gtest.h>

#include "jubatus/util/lang/cast.h"
#include "jubatus/util/text/json.h"
#include "jubatus/util/concurrent/thread.h"
#include "jubatus/util/lang/function.h"
#include "jubatus/util/lang/bind.h"
#include "jubatus/util/math/random.h"

#include "../storage/storage_type.hpp"
#include "../storage/local_storage.hpp"
#include "../storage/local_storage_mixture.hpp"
#include "../classifier/classifier_test_util.hpp"
#include "../classifier/classifier_factory.hpp"
#include "../classifier/classifier.hpp"
#include "../fv_converter/datum.hpp"
#include "../framework/stream_writer.hpp"
#include "../nearest_neighbor/nearest_neighbor_factory.hpp"
#include "classifier.hpp"

#include "test_util.hpp"

using std::string;
using std::vector;
using std::pair;
using std::make_pair;
using std::isfinite;
using std::numeric_limits;
using std::cout;
using std::endl;

using jubatus::util::text::json::json;
using jubatus::util::text::json::to_json;
using jubatus::util::text::json::json_object;
using jubatus::util::lang::lexical_cast;
using jubatus::util::lang::shared_ptr;
using jubatus::util::concurrent::thread;
using jubatus::util::lang::function;
using jubatus::util::lang::bind;
using jubatus::util::lang::_1;
using jubatus::util::lang::_2;
using jubatus::core::fv_converter::datum;
using jubatus::core::classifier::classify_result;
using jubatus::core::classifier::classifier_base;
using jubatus::core::storage::storage_base;

namespace jubatus {
namespace core {
namespace driver {

namespace {
datum convert_vector(const vector<double>& vec) {
  datum d;
  for (size_t i = 0; i < vec.size(); i++) {
    string f = "f" + lexical_cast<string>(i);
    d.num_values_.push_back(make_pair(f, vec[i]));
  }
  return d;
}

void make_random_data(
    jubatus::util::math::random::mtrand& rand,
    vector<pair<string, datum> >& data,
    size_t size) {
  for (size_t i = 0; i < size; ++i) {
    pair<string, vector<double> > p = gen_random_data(rand);
    data.push_back(make_pair(p.first, convert_vector(p.second)));
  }
}

string get_max_label(const classify_result& result) {
  string max_label = "";
  double max_prob = 0;
  for (size_t i = 0; i < result.size(); ++i) {
    if (max_label == "" || result[i].score > max_prob) {
      max_label = result[i].label;
      max_prob = result[i].score;
    }
  }
  return max_label;
}
}  // namespace

class classifier_test
    : public ::testing::TestWithParam<shared_ptr<classifier_base> > {
 protected:
  void SetUp() {
    classifier_.reset(new driver::classifier(
          GetParam(),
          make_fv_converter()));
  }

  void TearDown() {
    classifier_.reset();
  }

  void my_test();

  shared_ptr<core::driver::classifier> classifier_;
};

TEST_P(classifier_test, simple) {
  datum d;
  classifier_->train("hoge", d);
  classifier_->classify(d);
}

TEST_P(classifier_test, api_train) {
  jubatus::util::math::random::mtrand rand(0);
  const size_t example_size = 1000;

  vector<pair<string, datum> > data;
  make_random_data(rand, data, example_size);
  for (size_t i = 0; i < example_size; i++) {
    classifier_->train(data[i].first, data[i].second);
  }
}

void classifier_test::my_test() {
  jubatus::util::math::random::mtrand rand(0);
  const size_t example_size = 1000;

  vector<pair<string, datum> > data;
  make_random_data(rand, data, example_size);
  for (size_t i = 0; i < example_size; i++) {
    classifier_->train(data[i].first, data[i].second);
  }

  vector<string> labels;
  vector<classify_result> result;
  {
    vector<pair<string, datum> >::const_iterator it;
    for (it = data.begin(); it != data.end(); ++it) {
      labels.push_back(it->first);
      result.push_back(classifier_->classify(it->second));
    }
  }

  ASSERT_EQ(example_size, result.size());
  ASSERT_EQ(data.size(), result.size());

  vector<string>::const_iterator it0;  // answers
  vector<classify_result>::const_iterator it;
  size_t count = 0;
  for (it = result.begin(), it0 = labels.begin();
      it != result.end() && it0 != labels.end(); ++it, ++it0) {
    ASSERT_EQ(2u, it->size());  // estimate_results should have two label OK/NG
    string most0;
    double prob0 = DBL_MIN;
    classify_result::const_iterator ite;
    for (ite = it->begin(); ite != it->end(); ++ite) {
      // get most likely label
      if (prob0 < ite->score || ite == it->begin()) {
        prob0 = ite->score;
        most0 = ite->label;
      }
    }
    if (most0.compare(*it0) == 0) {
      count++;
    }
    // EXPECT_TRUE(*it0 == most0);
    if (most0.compare(*it0) != 0) {
      cout << *it0 << "!=" << most0 << endl;
      for (ite = it->begin(); ite != it->end(); ++ite) {
        cout << ite->label << "\t" << ite->score << endl;
      }
    }
  }
  // num of wrong classification should be less than 5%
  EXPECT_GE(count, result.size() - 50);
}

TEST_P(classifier_test, my_test) {
  my_test();
}

TEST_P(classifier_test, duplicated_keys) {
  jubatus::util::math::random::mtrand rand(0);
  datum d;
  for (size_t k = 0; k < 10; ++k) {
    uint32_t dim = rand.next_int(100);
    pair<string, double> feature = make_pair(lexical_cast<string>(dim), 1.0);
    // add 100 duplicated values
    for (size_t j = 0; j < 100; ++j)
    d.num_values_.push_back(feature);
  }
  for (size_t i = 0; i < 100; ++i) {
    string label = i % 2 == 0 ? "P" : "N";
    classifier_->train(label, d);
  }

  {
    datum d;
    for (size_t i = 0; i < 100; ++i) {
      d.num_values_.push_back(make_pair(lexical_cast<string>(i), 1.0));
    }
    classify_result result = classifier_->classify(d);
    /* if the classifier could not learn properly, it estimates
       scores of labels to NaN and returns no results. */
    ASSERT_EQ(2u, result.size());
  }
}

TEST_P(classifier_test, save_load) {
  jubatus::util::math::random::mtrand rand(0);
  const size_t example_size = 1000;

  vector<pair<string, datum> > data;
  make_random_data(rand, data, example_size);
  for (size_t i = 0; i < example_size; i++) {
    classifier_->train(data[i].first, data[i].second);
  }

  msgpack::sbuffer sbuf;
  framework::stream_writer<msgpack::sbuffer> st(sbuf);
  framework::jubatus_packer jp(st);
  framework::packer pk(jp);
  classifier_->pack(pk);

  classifier_->clear();

  msgpack::zone z;
  msgpack::object o = msgpack::unpack(z, sbuf.data(), sbuf.size());
  classifier_->unpack(o);

  my_test();
}

TEST_P(classifier_test, save_load_2) {
  msgpack::sbuffer save_empty, save_test;
  framework::stream_writer<msgpack::sbuffer>
    empty_sw(save_empty), test_sw(save_test);
  framework::jubatus_packer empty_jp(empty_sw), test_jp(test_sw);
  framework::packer empty_pk(empty_jp), test_pk(test_jp);

  // Test data
  datum pos;
  pos.num_values_.push_back(make_pair("value", 10.0));
  datum neg;
  neg.num_values_.push_back(make_pair("value", -10.0));

  // Save empty state
  classifier_->pack(empty_pk);

  // Train
  classifier_->train("pos", pos);
  classifier_->train("neg", neg);

  // Now, the classifier can classify properly
  ASSERT_EQ("pos", get_max_label(classifier_->classify(pos)));
  ASSERT_EQ("neg", get_max_label(classifier_->classify(neg)));

  // Save current state
  classifier_->pack(test_pk);

  // Load empty
  {
    msgpack::zone z;
    msgpack::object o = msgpack::unpack(z, save_empty.data(), save_empty.size());
    classifier_->unpack(o);
  }

  // And the classifier classify data improperly, but cannot expect results
  ASSERT_EQ(0u, classifier_->classify(pos).size());
  ASSERT_EQ(0u, classifier_->classify(neg).size());

  // Reload server
  {
    msgpack::zone z;
    msgpack::object o = msgpack::unpack(z, save_test.data(), save_test.size());
    classifier_->unpack(o);
  }

  // The classifier works well
  ASSERT_EQ("pos", get_max_label(classifier_->classify(pos)));
  ASSERT_EQ("neg", get_max_label(classifier_->classify(neg)));
}

TEST_P(classifier_test, save_load_3) {
  jubatus::util::math::random::mtrand rand(0);
  const size_t example_size = 1000;

  msgpack::sbuffer save_data;
  framework::stream_writer<msgpack::sbuffer> st(save_data);
  framework::jubatus_packer jp(st);
  framework::packer save_pk(jp);
  classifier_->pack(save_pk);

  vector<pair<string, datum> > data;
  make_random_data(rand, data, example_size);
  for (size_t i = 0; i < example_size; i++) {
    classifier_->train(data[i].first, data[i].second);
  }

  {
    msgpack::zone z;
    msgpack::object o = msgpack::unpack(z, save_data.data(), save_data.size());
    classifier_->unpack(o);
  }

  {
    EXPECT_EQ(0u, classifier_->get_labels().size());
  }

  {
    std::map<string, string> status;
    classifier_->get_status(status);
    EXPECT_EQ("0", status["num_classes"]);
  }

  my_test();
}

TEST_P(classifier_test, nan) {
  datum d;
  d.num_values_.push_back(
      make_pair("value", numeric_limits<float>::quiet_NaN()));
  classifier_->train("l1", d);


  classify_result result = classifier_->classify(d);
  ASSERT_EQ(1u, result.size());
  EXPECT_FALSE(isfinite(result[0].score));
}

vector<shared_ptr<classifier_base> > create_linear_classifiers() {
  vector<shared_ptr<classifier_base> > method;

  shared_ptr<core::storage::storage_base> storage;
  core::classifier::classifier_config config;

  storage.reset(new core::storage::local_storage);
  method.push_back(shared_ptr<classifier_base>(
          new core::classifier::perceptron(storage)));

  storage.reset(new core::storage::local_storage_mixture);
  method.push_back(shared_ptr<classifier_base>(
          new core::classifier::perceptron(storage)));

  storage.reset(new core::storage::local_storage);
  method.push_back(shared_ptr<classifier_base>(
          new core::classifier::passive_aggressive(storage)));

  storage.reset(new core::storage::local_storage_mixture);
  method.push_back(shared_ptr<classifier_base>(
          new core::classifier::passive_aggressive(storage)));

  storage.reset(new core::storage::local_storage);
  method.push_back(shared_ptr<classifier_base>(
          new core::classifier::passive_aggressive_1(config, storage)));

  storage.reset(new core::storage::local_storage_mixture);
  method.push_back(shared_ptr<classifier_base>(
          new core::classifier::passive_aggressive_1(config, storage)));

  storage.reset(new core::storage::local_storage);
  method.push_back(shared_ptr<classifier_base>(
          new core::classifier::passive_aggressive_2(config, storage)));

  storage.reset(new core::storage::local_storage_mixture);
  method.push_back(shared_ptr<classifier_base>(
          new core::classifier::passive_aggressive_2(config, storage)));

  storage.reset(new core::storage::local_storage);
  method.push_back(shared_ptr<classifier_base>(
          new core::classifier::confidence_weighted(config, storage)));

  storage.reset(new core::storage::local_storage_mixture);
  method.push_back(shared_ptr<classifier_base>(
          new core::classifier::confidence_weighted(config, storage)));

  storage.reset(new core::storage::local_storage);
  method.push_back(shared_ptr<classifier_base>(
          new core::classifier::arow(config, storage)));

  storage.reset(new core::storage::local_storage_mixture);
  method.push_back(shared_ptr<classifier_base>(
          new core::classifier::arow(config, storage)));

  config.regularization_weight = 0.1f;
  storage.reset(new core::storage::local_storage);
  method.push_back(shared_ptr<classifier_base>(
          new core::classifier::normal_herd(config, storage)));

  storage.reset(new core::storage::local_storage_mixture);
  method.push_back(shared_ptr<classifier_base>(
          new core::classifier::normal_herd(config, storage)));

  return method;
}

vector<shared_ptr<classifier_base> > create_nn_classifiers() {
  vector<shared_ptr<classifier_base> > method;

  json js(new json_object);
  js["hash_num"] = to_json(64);
  core::common::jsonconfig::config conf(js);
  shared_ptr<storage::column_table> table(new storage::column_table);
  shared_ptr<nearest_neighbor::nearest_neighbor_base>
      nearest_neighbor_engine(nearest_neighbor::create_nearest_neighbor(
          "lsh", conf, table, ""));
  method.push_back(shared_ptr<classifier_base>(
      new core::classifier::nearest_neighbor_classifier(
          nearest_neighbor_engine, 3, 1.f)));

  return method;
}

INSTANTIATE_TEST_CASE_P(classifier_test_instance,
    classifier_test,
    testing::ValuesIn(create_linear_classifiers()));

class concurrent_classifier_test
    : public ::testing::TestWithParam<shared_ptr<classifier_base> > {
 protected:
  void SetUp() {
    classifier_.reset(new driver::classifier(
          GetParam(),
          make_tf_idf_fv_converter()));
  }

  void TearDown() {
    classifier_.reset();
  }

  void my_test();

  shared_ptr<core::driver::classifier> classifier_;
};

void train_task(int thread_id,
                 int num,
                 shared_ptr<core::driver::classifier> target) {
  jubatus::util::math::random::mtrand rand;
  for (int i = 0; i < num; i++) {
    target->train(rand() & 1 ? "ok" : "ng",
                  generate_random_datum(rand, 10));
  }
}

void classify_task(int thread_id,
                   int num,
                   shared_ptr<core::driver::classifier> target) {
  jubatus::util::math::random::mtrand rand;
  vector<pair<string, datum> > data;
  make_random_data(rand, data, num);
  for (int i = 0; i < num; i++) {
    target->classify(data[i].second);
  }
}

void pack_task(int thread_id,
               int num,
               shared_ptr<core::driver::classifier> target) {
  for (int i = 0; i < num; i++) {
    msgpack::sbuffer user_data_buf;
    core::framework::stream_writer<msgpack::sbuffer> st(user_data_buf);
    core::framework::jubatus_packer jp(st);
    core::framework::packer packer(jp);
    target->pack(packer);
  }
}

void set_label_task(int thread_id,
                 int num,
                 shared_ptr<core::driver::classifier> target) {
  jubatus::util::math::random::mtrand rand;
  vector<string> labels;
  for (int i = 0; i < 100; ++i) {
    labels.push_back(generate_random_string(rand, 10));
  }
  for (int i = 0; i < num; i++) {
    target->set_label(labels[rand.next_int(labels.size())]);
  }
}

TEST_P(concurrent_classifier_test, train_train) {
  thread t0(bind(train_task, 0, 200, classifier_));
  thread t1(bind(train_task, 1, 200, classifier_));
  t0.start();
  t1.start();
  t0.join();
  t1.join();
}

TEST_P(concurrent_classifier_test, train_classify) {
  thread t0(bind(train_task,    0, 200, classifier_));
  thread t1(bind(classify_task, 1, 200, classifier_));
  t0.start();
  t1.start();
  t0.join();
  t1.join();
}

TEST_P(concurrent_classifier_test, classify_classify) {
  train_task(0, 100, classifier_);
  thread t0(bind(classify_task, 0, 200, classifier_));
  thread t1(bind(classify_task, 1, 200, classifier_));
  t0.start();
  t1.start();
  t0.join();
  t1.join();
}

TEST_P(concurrent_classifier_test, classify_train_save) {
  train_task(0, 100, classifier_);
  thread t0(bind(classify_task, 0, 200, classifier_));
  thread t1(bind(train_task, 1, 200, classifier_));
  thread t2(bind(pack_task, 2, 100, classifier_));
  t0.start();
  t1.start();
  t2.start();
  t0.join();
  t1.join();
  t2.join();
}

TEST_P(concurrent_classifier_test, classify_train_set_label) {
  train_task(0, 100, classifier_);
  thread t0(bind(classify_task, 0, 200, classifier_));
  thread t1(bind(train_task, 1, 200, classifier_));
  thread t2(bind(set_label_task, 2, 200, classifier_));
  t0.start();
  t1.start();
  t2.start();
  t0.join();
  t1.join();
  t2.join();
}


INSTANTIATE_TEST_CASE_P(concurrent_classifier_test_instance,
                        concurrent_classifier_test,
                        testing::ValuesIn(create_linear_classifiers()));

INSTANTIATE_TEST_CASE_P(concurrent_nn_classifier_test_instance,
                        concurrent_classifier_test,
                        testing::ValuesIn(create_nn_classifiers()));
}  // driver namespace
}  // core namespace
}  // jubatus namespace
