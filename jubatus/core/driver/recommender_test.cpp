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


#include <algorithm>
#include <vector>
#include <utility>
#include <map>
#include <string>

#include <gtest/gtest.h>

#include "jubatus/util/lang/cast.h"
#include "jubatus/util/lang/shared_ptr.h"
#include "../common/jsonconfig.hpp"

#include "../fv_converter/datum.hpp"
#include "../fv_converter/converter_config.hpp"
#include "../recommender/recommender.hpp"
#include "../recommender/recommender_type.hpp"
#include "../recommender/recommender_factory.hpp"
#include "../classifier/classifier_test_util.hpp"
#include "../framework/stream_writer.hpp"
#include "../storage/column_table.hpp"
#include "../unlearner/lru_unlearner.hpp"
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
using jubatus::util::text::json::to_json;
using jubatus::util::lang::lexical_cast;
using jubatus::core::fv_converter::datum;
using jubatus::core::recommender::recommender_base;
using jubatus::core::storage::column_table;
using jubatus::core::unlearner::unlearner_base;
using jubatus::core::unlearner::lru_unlearner;
using jubatus::core::recommender::inverted_index;

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

TEST_F(recommender_test, similar_row_from) {
  datum d1, d2;
  d1.num_values_.push_back(make_pair("f1", 1.0));
  d2.num_values_.push_back(make_pair("f1", 1.0));
  d2.num_values_.push_back(make_pair("f2", 1.0));

  recommender_->update_row("id1", d1);
  recommender_->update_row("id2", d1);
  recommender_->update_row("id3", d1);
  recommender_->update_row("id4", d2);
  recommender_->update_row("id5", d2);

  vector<pair<string, double> > ret;

  ret = recommender_->similar_row_from_datum(d1, 5);
  ASSERT_EQ(5, ret.size());

  ret = recommender_->similar_row_from_id("id1", 5);
  ASSERT_EQ(5, ret.size());

  ret = recommender_->similar_row_from_datum_and_score(d1, 0.8);
  ASSERT_EQ(3, ret.size());  // id1, id2, id3

  ret = recommender_->similar_row_from_id_and_score("id4", 0.8);
  ASSERT_EQ(2, ret.size());  // id4, id5

  ret = recommender_->similar_row_from_datum_and_rate(d1, 0.6);
  ASSERT_EQ(3, ret.size());  // id1, id2, id3

  ret = recommender_->similar_row_from_id_and_rate("id4", 0.2);
  ASSERT_EQ(1, ret.size());  // id4
}

TEST_F(recommender_test, validate_rate) {
  datum d1;
  d1.num_values_.push_back(make_pair("f1", 1.0));

  recommender_->update_row("id1", d1);

  // similar_row_from_datum_and_rate

  ASSERT_THROW(
    recommender_->similar_row_from_datum_and_rate(d1, -0.1),
    common::invalid_parameter);

  ASSERT_THROW(
    recommender_->similar_row_from_datum_and_rate(d1, 0),
    common::invalid_parameter);

  ASSERT_NO_THROW(
    recommender_->similar_row_from_datum_and_rate(d1, 1));

  ASSERT_THROW(
    recommender_->similar_row_from_datum_and_rate(d1, 1.1),
    common::invalid_parameter);

  // similar_row_from_id_and_rate

  ASSERT_THROW(
    recommender_->similar_row_from_id_and_rate("id1", -0.1),
    common::invalid_parameter);

  ASSERT_THROW(
    recommender_->similar_row_from_id_and_rate("id1", 0),
    common::invalid_parameter);

  ASSERT_NO_THROW(
    recommender_->similar_row_from_id_and_rate("id1", 1));

  ASSERT_THROW(
    recommender_->similar_row_from_id_and_rate("id1", 1.1),
    common::invalid_parameter);
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

  vector<pair<string, double> > ret =
      recommender_->similar_row_from_datum(d, 10);
  ASSERT_EQ("id4", ret[0].first);
}

TEST_P(nn_recommender_test, missing_id) {
  recommender_->update_row("id1", create_datum_str("a", "a b c"));

  vector<pair<string, double> > ret =
      recommender_->similar_row_from_id("id2", 10);
  ASSERT_EQ(0, ret.size());
}

INSTANTIATE_TEST_CASE_P(nn_recommender_test_instance,
    nn_recommender_test,
    testing::ValuesIn(create_recommender_bases()));

class recommender_with_unlearning_test
    : public ::testing::TestWithParam<pair<string,
        common::jsonconfig::config> > {
 protected:
  shared_ptr<driver::recommender> create_driver() const {
    const string id("my_id");
    return shared_ptr<driver::recommender>(
        new driver::recommender(
            core::recommender::recommender_factory::create_recommender(
                GetParam().first, GetParam().second, id),
        make_tf_idf_fv_converter()));
  }

  void SetUp() {
    recommender_ = create_driver();
  }

  void TearDown() {
    recommender_->clear();
    recommender_.reset();
  }

  shared_ptr<driver::recommender> recommender_;
};

const size_t MAX_SIZE = 3;

vector<pair<string, common::jsonconfig::config> >
create_recommender_configs_with_unlearner() {
  vector<pair<string, common::jsonconfig::config> > configs;

  json js(new json_object);
  js["unlearner"] = to_json(string("lru"));
  js["unlearner_parameter"] = new json_object;
  js["unlearner_parameter"]["max_size"] = to_json(MAX_SIZE);
  js["unlearner_parameter"]["sticky_pattern"] =
    to_json(string("*_sticky"));

  // inverted_index
  configs.push_back(make_pair("inverted_index",
      common::jsonconfig::config(js)));

  // inverted_index_euclid
  configs.push_back(make_pair("inverted_index_euclid",
      common::jsonconfig::config(js)));

  // lsh
  json js_lsh(js.clone());
  js_lsh["hash_num"] = to_json(64);
  configs.push_back(make_pair("lsh", common::jsonconfig::config(js_lsh)));

  // minhash
  json js_minhash(js.clone());
  js_minhash["hash_num"] = to_json(64);
  configs.push_back(
      make_pair("minhash", common::jsonconfig::config(js_minhash)));

  // euclid_lsh
  json js_euclid_lsh(js.clone());
  js_euclid_lsh["hash_num"] = to_json(64);
  js_euclid_lsh["table_num"] = to_json(4);
  js_euclid_lsh["seed"] = to_json(1091);
  js_euclid_lsh["probe_num"] = to_json(64);
  js_euclid_lsh["bin_width"] = to_json(100);
  configs.push_back(
      make_pair("euclid_lsh", common::jsonconfig::config(js_euclid_lsh)));

  // TODO(@rimms): Add NN-based algorithm

  return configs;
}

TEST_P(recommender_with_unlearning_test, update_row) {
  recommender_->update_row("id1", create_datum_str("a", "b c"));
  recommender_->update_row("id2", create_datum_str("a", "c d"));
  recommender_->update_row("id3", create_datum_str("a", "d e"));
  recommender_->update_row("id4", create_datum_str("a", "e f"));
  recommender_->update_row("id5", create_datum_str("a", "f g"));
  recommender_->update_row("id6", create_datum_str("a", "g h"));
  recommender_->update_row("id7", create_datum_str("a", "h i"));

  vector<pair<string, double> > ret =
    recommender_->similar_row_from_id("id6", MAX_SIZE + 1);
  ASSERT_EQ(MAX_SIZE, ret.size());
}

TEST_P(recommender_with_unlearning_test, clear_row) {
  recommender_->update_row("id1", create_datum_str("a", "b c"));
  recommender_->update_row("id2", create_datum_str("a", "c d"));
  recommender_->update_row("id3", create_datum_str("a", "d e"));
  recommender_->clear_row("id1");
  recommender_->update_row("id4", create_datum_str("a", "e f"));
  recommender_->update_row("id5", create_datum_str("a", "f g"));
  recommender_->update_row("id6", create_datum_str("a", "g h"));
  recommender_->update_row("id7", create_datum_str("a", "h i"));

  vector<pair<string, double> > ret =
    recommender_->similar_row_from_id("id6", MAX_SIZE + 1);
  ASSERT_EQ(MAX_SIZE, ret.size());

  vector<string> all = recommender_->get_all_rows();
  ASSERT_EQ(MAX_SIZE, all.size());
}

TEST_P(recommender_with_unlearning_test, missing_id) {
  recommender_->update_row("id1", create_datum_str("a", "b c"));

  vector<pair<string, double> > ret =
    recommender_->similar_row_from_id("id2", MAX_SIZE + 1);
  ASSERT_EQ(0, ret.size());
}

INSTANTIATE_TEST_CASE_P(
    recommender_with_unlearning_test_instance,
    recommender_with_unlearning_test,
    testing::ValuesIn(create_recommender_configs_with_unlearner()));

class recommender_mix_with_unlearning_test
    : public ::testing::TestWithParam<pair<string,
        common::jsonconfig::config> > {
 protected:
  shared_ptr<driver::recommender> create_driver(const string& id) const {
    return shared_ptr<driver::recommender>(
        new driver::recommender(
            core::recommender::recommender_factory::create_recommender(
                GetParam().first, GetParam().second, id),
        make_fv_converter()));
  }

  virtual void SetUp() {
    recommender1 = create_driver("my_id1");
    recommender2 = create_driver("my_id2");

    mixable1 =
        dynamic_cast<framework::linear_mixable*>(recommender1->get_mixable());
    ASSERT_TRUE(mixable1 != NULL);
    mixable2 =
        dynamic_cast<framework::linear_mixable*>(recommender2->get_mixable());
    ASSERT_TRUE(mixable2 != NULL);
  }

  virtual void TearDown() {
    recommender1->clear();
    recommender1.reset();
    recommender2->clear();
    recommender2.reset();
  }

  framework::diff_object make_diff() {
    msgpack::sbuffer data1;
    msgpack::unpacked unpacked1;
    {
      core::framework::stream_writer<msgpack::sbuffer> st(data1);
      core::framework::jubatus_packer jp(st);
      core::framework::packer pk(jp);
      mixable1->get_diff(pk);
      msgpack::unpack(&unpacked1, data1.data(), data1.size());
    }

    msgpack::sbuffer data2;
    msgpack::unpacked unpacked2;
    {
      core::framework::stream_writer<msgpack::sbuffer> st(data2);
      core::framework::jubatus_packer jp(st);
      core::framework::packer pk(jp);
      mixable2->get_diff(pk);
      msgpack::unpack(&unpacked2, data2.data(), data2.size());
    }
    framework::diff_object diff =
        mixable2->convert_diff_object(unpacked2.get());
    mixable2->mix(unpacked1.get(), diff);
    return diff;
  }
  shared_ptr<driver::recommender> recommender1, recommender2;
  framework::linear_mixable *mixable1, *mixable2;
};

TEST_P(recommender_mix_with_unlearning_test, basic) {
  recommender1->update_row("id1", create_datum_str("a", "a b c"));
  recommender1->update_row("id2", create_datum_str("a", "d e f"));
  recommender1->update_row("id3", create_datum_str("a", "e f g"));

  recommender2->update_row("id2", create_datum_str("a", "d e f"));
  recommender2->update_row("id3", create_datum_str("a", "e f g"));
  recommender2->update_row("id4", create_datum_str("a", "f g h"));

  framework::diff_object diff = make_diff();

  mixable1->put_diff(diff);
  mixable2->put_diff(diff);

  ASSERT_EQ(3u, recommender1->get_all_rows().size());
  ASSERT_EQ(3u, recommender2->get_all_rows().size());
}

TEST_P(recommender_mix_with_unlearning_test, mix_all) {
  recommender1->update_row("id1", create_datum_str("a", "a b c"));
  recommender1->update_row("id2", create_datum_str("a", "d e f"));
  recommender1->update_row("id3", create_datum_str("a", "e f g"));

  recommender2->update_row("id4", create_datum_str("a", "d e f"));
  recommender2->update_row("id5", create_datum_str("a", "e f g"));
  recommender2->update_row("id6", create_datum_str("a", "f g h"));

  framework::diff_object diff = make_diff();

  {
    std::cout << "mixable1" << std::endl;
    mixable1->put_diff(diff);
    vector<string> all_row1 = recommender1->get_all_rows();
    std::cout << "having1: {";
    for (size_t i = 0; i < all_row1.size(); ++i) {
      std::cout << all_row1[i] << ", ";
    }
    std::cout << "}" << std::endl;
    ASSERT_EQ(3u, all_row1.size());
  }
  {
    std::cout << "mixable2" << std::endl;
    mixable2->put_diff(diff);
    vector<string> all_row2 = recommender2->get_all_rows();
    std::cout << "having2: {";
    for (size_t i = 0; i < all_row2.size(); ++i) {
      std::cout << all_row2[i] << ", ";
    }
    std::cout << "}" << std::endl;
    ASSERT_EQ(3u, all_row2.size());
  }
}

TEST_P(recommender_mix_with_unlearning_test, all_sticky) {
  recommender1->update_row("id1_sticky", create_datum_str("a", "a b c"));
  recommender1->update_row("id2_sticky", create_datum_str("a", "d e f"));
  recommender1->update_row("id3_sticky", create_datum_str("a", "e f g"));

  recommender2->update_row("id4_sticky", create_datum_str("a", "d e f"));
  recommender2->update_row("id5_sticky", create_datum_str("a", "e f g"));
  recommender2->update_row("id6_sticky", create_datum_str("a", "f g h"));

  framework::diff_object diff = make_diff();

  {
    std::cout << "mixable1" << std::endl;
    mixable1->put_diff(diff);

    vector<string> all_rows1 = recommender1->get_all_rows();
    std::cout << "having1: {";
    for (size_t i = 0; i < all_rows1.size(); ++i) {
      std::cout << all_rows1[i] << ", ";
    }
    std::cout << "}" << std::endl;
    ASSERT_EQ(3u, all_rows1.size());
    std::sort(all_rows1.begin(), all_rows1.end());
    ASSERT_EQ("id1_sticky", all_rows1[0]);
    ASSERT_EQ("id2_sticky", all_rows1[1]);
    ASSERT_EQ("id3_sticky", all_rows1[2]);
  }

  {
    std::cout << "mixable2" << std::endl;
    mixable2->put_diff(diff);

    vector<string> all_rows2 = recommender2->get_all_rows();
    std::cout << "having2: {";
    for (size_t i = 0; i < all_rows2.size(); ++i) {
      std::cout << all_rows2[i] << ", ";
    }
    std::cout << "}" << std::endl;
    ASSERT_EQ(3u, all_rows2.size());
    std::sort(all_rows2.begin(), all_rows2.end());
    ASSERT_EQ("id4_sticky", all_rows2[0]);
    ASSERT_EQ("id5_sticky", all_rows2[1]);
    ASSERT_EQ("id6_sticky", all_rows2[2]);
  }
}

// TODO(kumagi): append test if there are all sticky rows

INSTANTIATE_TEST_CASE_P(
    recommender_mix_with_lru_unlearning_test_instance,
    recommender_mix_with_unlearning_test,
    testing::ValuesIn(create_recommender_configs_with_unlearner()));

}  // namespace driver
}  // namespace core
}  // namespace jubatus
