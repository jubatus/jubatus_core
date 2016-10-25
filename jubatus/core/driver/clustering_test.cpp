// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2014 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#include <math.h>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <utility>
#include <gtest/gtest.h>

#include "clustering.hpp"

#include "jubatus/util/lang/shared_ptr.h"
#include "jubatus/util/math/random.h"
#include "jubatus/util/lang/cast.h"
#include "../clustering/clustering.hpp"
#include "../clustering/types.hpp"
#include "../clustering/kmeans_clustering_method.hpp"
#include "../clustering/gmm_clustering_method.hpp"
#include "../clustering/clustering_factory.hpp"
#include "../framework/stream_writer.hpp"
#include "test_util.hpp"
#include "../fv_converter/datum.hpp"
#include "../common/jsonconfig.hpp"

using std::vector;
using std::string;
using std::pair;
using std::set;
using std::map;
using std::make_pair;
using jubatus::util::lang::shared_ptr;
using jubatus::util::lang::lexical_cast;
using jubatus::core::fv_converter::datum;
using jubatus::core::fv_converter::datum_to_fv_converter;
using jubatus::core::clustering::clustering_method;
using jubatus::util::text::json::json;
using jubatus::util::text::json::json_object;
using jubatus::util::text::json::to_json;

namespace jubatus {
namespace core {
namespace driver {

struct clustering_config {
  clustering_config()
    : k(2),
      bucket_size(10000),
      bucket_length(2),
      bicriteria_base_size(10),
      compressed_bucket_size(200),
      forgetting_factor(2.0),
      forgetting_threshold(0.05),
      seed(0),
      eps(2.0),
      min_core_point(1) {
  }
  int k;
  int bucket_size;
  int bucket_length;
  int bicriteria_base_size;
  int compressed_bucket_size;
  double forgetting_factor;
  double forgetting_threshold;
  int64_t seed;
  double eps;
  int min_core_point;
};

common::jsonconfig::config make_simple_config() {
  json js(new json_object);
  js = new json_object;
  js["bucket_size"] = to_json(200);
  common::jsonconfig::config conf(js);
  return conf;
}

common::jsonconfig::config make_simple_config_idf() {
  json js(new json_object);
  js = new json_object;
  js["bucket_size"] = to_json(50);
  common::jsonconfig::config conf(js);
  return conf;
}

common::jsonconfig::config make_compressive_config() {
    json js(new json_object);
    js = new json_object;
    js["bucket_size"] = to_json(200);
    js["bucket_length"] = to_json(2);
    js["compressed_bucket_size"] = to_json(20);
    js["bicriteria_base_size"] = to_json(2);
    js["forgetting_factor"] = to_json(0.0);
    js["forgetting_threshold"] = to_json(0.5);
    js["seed"] = to_json(0);
    common::jsonconfig::config conf(js);
    return conf;
}

common::jsonconfig::config make_compressive_config_idf() {
    json js(new json_object);
    js = new json_object;
    js["bucket_size"] = to_json(50);
    js["bucket_length"] = to_json(2);
    js["compressed_bucket_size"] = to_json(10);
    js["bicriteria_base_size"] = to_json(5);
    js["forgetting_factor"] = to_json(0.0);
    js["forgetting_threshold"] = to_json(0.5);
    js["seed"] = to_json(0);
    common::jsonconfig::config conf(js);
    return conf;
}

common::jsonconfig::config make_kmeans_config() {
  json js(new json_object);
  js["k"] = to_json(2);
  js["seed"] = to_json(0);
  common::jsonconfig::config conf(js);
  return conf;
}

common::jsonconfig::config make_gmm_config() {
  json js(new json_object);
  js["k"] = to_json(2);
  js["seed"] = to_json(0);
  common::jsonconfig::config conf(js);
  return conf;
}

common::jsonconfig::config make_dbscan_config() {
  json js(new json_object);
  js["eps"] = to_json(100.0);
  js["min_core_point"] = to_json(2);
  common::jsonconfig::config conf(js);
  return conf;
}


class clustering_test
    : public ::testing::TestWithParam<pair<string, string> > {

 protected:
  shared_ptr<driver::clustering> create_driver() const {
    common::jsonconfig::config config;
    common::jsonconfig::config compressor_config;
    pair<string, string> param = GetParam();
    if (param.first == "simple") {
      compressor_config = make_simple_config();
    } else {
      compressor_config = make_compressive_config();
    }

    if (param.second == "kmeans") {
      config = make_kmeans_config();
    } else if (param.second == "gmm") {
      config = make_gmm_config();
    } else {
      config = make_dbscan_config();
    }
    return shared_ptr<driver::clustering>(new driver::clustering(
               core::clustering::clustering_factory::create(
                                                            "dummy",
                                                            param.second,
                                                            param.first,
                                                            config,
                                                            compressor_config),
               make_fv_converter()));
  }
  void SetUp() {
    pair<string, string> param = GetParam();
    conf_.k = 2;
    conf_.bucket_size = 200;
    conf_.compressed_bucket_size = conf_.bucket_size / 10;
    conf_.bicriteria_base_size = conf_.bucket_size / 100;
    conf_.bucket_length = 2;
    conf_.forgetting_factor = 0.0;
    conf_.forgetting_threshold = 0.5;
    conf_.eps = 100.0;
    conf_.min_core_point = 2;
    clustering_ = create_driver();
    method_ = param.second;
  }
  void TearDown() {
    clustering_.reset();
  }
  shared_ptr<driver::clustering> clustering_;
  string compressor_method_;
  string method_;
  clustering_config conf_;
};

namespace {  // testing util
datum single_datum(string key, double v) {
  datum d;
  d.num_values_.push_back(make_pair(key, v));
  return d;
}
core::clustering::indexed_point single_indexed_point(string id, datum datum) {
  core::clustering::indexed_point p;
  p.id = id;
  p.point = datum;
  return p;
}
}

TEST_P(clustering_test, get_revision) {
  const int num = conf_.bucket_size * 10;
  for (int i = 0; i < num; ++i) {
    vector<core::clustering::indexed_point> points;
    points.push_back(single_indexed_point(
        lexical_cast<string>(i), single_datum("a", 1)));
    clustering_->push(points);
  }
  std::size_t expected = num / conf_.bucket_size;
  ASSERT_EQ(expected, clustering_->get_revision());
}

TEST_P(clustering_test, push) {
  for (int j = 0; j < conf_.bucket_size / 5; ++j) {
    vector<core::clustering::indexed_point> points;
    for (int i = 0; i < 100; i += 5) {
      points.push_back(single_indexed_point(
          lexical_cast<string>(i), single_datum("a", i * 2)));
      points.push_back(single_indexed_point(
          lexical_cast<string>(i), single_datum("a", i * 2)));
    }
    clustering_->push(points);
  }
}

TEST_P(clustering_test, push_indexed_point) {
  vector<core::clustering::indexed_point> points;
  const int num = conf_.bucket_size * 10;
  int skip = 5;
  for (int i = 0; i < num; i += skip) {
    datum d = single_datum("a", i * 2);
    points.push_back(single_indexed_point(lexical_cast<string>(i), d));
  }
  clustering_->push(points);
  std::size_t expected = num / skip / conf_.bucket_size;
  ASSERT_EQ(expected, clustering_->get_revision());
}

TEST_P(clustering_test, save_load) {
  {
    core::fv_converter::datum d;
    vector<core::clustering::indexed_point> points;
    points.push_back(single_indexed_point(
        lexical_cast<string>(1), single_datum("a", 1)));
    clustering_->push(points);
  }

  // save to a buffer
  msgpack::sbuffer sbuf;
  framework::stream_writer<msgpack::sbuffer> st(sbuf);
  framework::jubatus_packer jp(st);
  framework::packer packer(jp);
  clustering_->pack(packer);

  // restart the driver
  TearDown();
  SetUp();

  // unpack the buffer
  msgpack::unpacked unpacked;
  msgpack::unpack(&unpacked, sbuf.data(), sbuf.size());
  clustering_->unpack(unpacked.get());
}

TEST_P(clustering_test, clear) {
  const int num = conf_.bucket_size * 10;
  for (int i = 0; i < num; ++i) {
    vector<core::clustering::indexed_point> points;
    points.push_back(single_indexed_point(
        lexical_cast<string>(i), single_datum("a", 1)));
    clustering_->push(points);
  }

  clustering_->clear();

  ASSERT_THROW(
      clustering_->get_core_members(),
      core::clustering::not_performed);

  ASSERT_EQ(0u, clustering_->get_revision());
}

TEST_P(clustering_test, get_k_center) {
  if (method_ == "dbscan") {
      ASSERT_THROW(clustering_->get_k_center(), common::unsupported_method);
      return;
    }

  jubatus::util::math::random::mtrand r(0);

  for (int j = 0; j < conf_.bucket_size; ++j) {
    datum a, b;
    a.num_values_.push_back(make_pair("a", -100 + r.next_gaussian() * 20));
    a.num_values_.push_back(make_pair("b", -200 + r.next_gaussian() * 100));
    b.num_values_.push_back(make_pair("c", 50000 + r.next_gaussian() * 100));
    b.num_values_.push_back(make_pair("d", 250000 + r.next_gaussian() * 500));

    vector<core::clustering::indexed_point> one;
    one.push_back(single_indexed_point("1", a));
    clustering_->push(one);
    one.clear();

    vector<core::clustering::indexed_point> two;
    two.push_back(single_indexed_point("2", b));
    clustering_->push(two);
    two.clear();
  }

  clustering_->do_clustering();
  {
    vector<datum> result = clustering_->get_k_center();
    ASSERT_EQ(std::size_t(conf_.k), result.size());
    ASSERT_LT(1U, result[0].num_values_.size());

    // two center should be far about sqrt(100^2 + 200^2 + 5000^2 + 250000^2)
    double squared_diff = 0;
    std::vector<map<string, double> > centers;

    for (size_t i = 0; i < result.size(); ++i) {
      map<string, double> center;
      const vector<pair<string, double> >& num_values = result[i].num_values_;
      for (size_t j = 0; j < num_values.size(); ++j) {
        center.insert(num_values[j]);
      }
      centers.push_back(center);
    }
    squared_diff += std::pow(centers[0]["a"] - centers[1]["a"], 2);
    squared_diff += std::pow(centers[0]["b"] - centers[1]["b"], 2);
    squared_diff += std::pow(centers[0]["c"] - centers[1]["c"], 2);
    squared_diff += std::pow(centers[0]["d"] - centers[1]["d"], 2);

    const double min_diff =
        std::pow(80, 2)    + std::pow(100, 2) +
        std::pow(49900, 2) + std::pow(249500, 2);
    ASSERT_LT(min_diff * 0.8, squared_diff);
  }
}

TEST_P(clustering_test, integer_center) {
  if (method_ == "dbscan") {  // dbscan does not have center. so pass this test
    return;
  }

  jubatus::util::math::random::mtrand r(0);
  const int quantity = conf_.bucket_size * 5;
  std::vector<core::clustering::indexed_point> data(quantity);

  for (int i = 0; i < quantity ; ++i) {
    data[i].point.num_values_.push_back(
        make_pair("x", 100 + r.next_int(-10, 10)));
    data[i].id = lexical_cast<string>(i);
  }

  clustering_->push(data);
  const std::vector<fv_converter::datum> centers = clustering_->get_k_center();

  ASSERT_EQ(std::size_t(conf_.k), centers.size());
}

struct check_points {
  float a;
  float b;
  check_points(float a_, float b_)
    : a(a_), b(b_) {}
  bool operator<(const check_points& rhs) const {
    if (a < rhs.a) {
      return true;
    } else if (rhs.a < a) {
      return false;
    } else if (b < rhs.b) {
      return true;
    } else {
      return false;
    }
  }
};

struct check_point_compare {
  bool operator()(const check_points& lhs, const check_points& rhs) {
    return lhs < rhs;
  }
};

TEST_P(clustering_test, get_nearest_members) {
  jubatus::util::math::random::mtrand r(0);
  set<check_points, check_point_compare> points;

  for (int i = 0; i < conf_.bucket_size * 2 + 1; ++i) {
    datum x, y;
    float a = 100 + r.next_gaussian() * 20;
    float b = 1000 + r.next_gaussian() * 400;
    points.insert(check_points(a, b));

    x.num_values_.push_back(make_pair("a", a));
    x.num_values_.push_back(make_pair("b", b));
    y.num_values_.push_back(make_pair("c", -5000 - r.next_gaussian() * 100));
    y.num_values_.push_back(make_pair("d", -1000 - r.next_gaussian() * 50));

    vector<core::clustering::indexed_point> one;
    one.push_back(single_indexed_point(lexical_cast<string>(2*i), x));
    clustering_->push(one);
    one.clear();

    vector<core::clustering::indexed_point> two;
    two.push_back(single_indexed_point(lexical_cast<string>(2*i+1), y));
    clustering_->push(two);
    two.clear();
  }

  clustering_->do_clustering();

  {
    if (method_ == "dbscan") {
      ASSERT_THROW(clustering_->get_k_center(), common::unsupported_method);
    } else {
      vector<datum> result = clustering_->get_k_center();
      ASSERT_EQ(std::size_t(conf_.k), result.size());
    }
  }

  set<check_points, check_point_compare>::const_iterator it;
  for (it = points.begin(); it != points.end(); ++it) {
    datum x;
    x.num_values_.push_back(make_pair("a", it->a));
    x.num_values_.push_back(make_pair("b", it->b));

    if (method_ == "dbscan") {
      ASSERT_THROW(clustering_->
                   get_nearest_members(x), common::unsupported_method);
      return;
    }

    core::clustering::cluster_unit result =
        clustering_->get_nearest_members(x);

    ASSERT_LT(1u, result.size());
    for (size_t i = 0; i < result.size(); ++i) {
      const vector<pair<string, double> >& near_points =
          result[i].second.num_values_;
      ASSERT_EQ(2u, near_points.size());  // must be 2-dimentional

      map<string, double> point;
      for (size_t j = 0; j < near_points.size(); ++j) {
        point.insert(near_points[j]);
      }

      if (point.count("c") == 0) {
        // the point is in cluster of (a, b) dimension
        ASSERT_NE(0.0, point["a"]);
        ASSERT_NE(0.0, point["b"]);
        ASSERT_EQ(0.0, point["c"]);
        ASSERT_EQ(0.0, point["d"]);
      } else if (point.count("a") == 0) {
        // the point is in cluster of (c, d) dimension
        ASSERT_EQ(0.0, point["a"]);
        ASSERT_EQ(0.0, point["b"]);
        ASSERT_NE(0.0, point["c"]);
        ASSERT_NE(0.0, point["d"]);
      } else {
        std::cout << "{";
        for (map<string, double>::const_iterator it = point.begin();
             it != point.end();
             ++it) {
          std::cout << it->first << ":" << it->second << ", ";
        }
        std::cout << "}";
        ASSERT_FALSE("invalid center");
      }
    }
  }
}

TEST_P(clustering_test, get_nearest_members_light) {
  jubatus::util::math::random::mtrand r(0);

  for (int i = 0; i < conf_.bucket_size * 2 + 1; ++i) {
    datum x, y;
    float a = 100 + r.next_gaussian() * 20;
    float b = 1000 + r.next_gaussian() * 400;

    x.num_values_.push_back(make_pair("a", a));
    x.num_values_.push_back(make_pair("b", b));
    y.num_values_.push_back(make_pair("c", -5000 - r.next_gaussian() * 100));
    y.num_values_.push_back(make_pair("d", -1000 - r.next_gaussian() * 50));

    vector<core::clustering::indexed_point> one;
    one.push_back(single_indexed_point(lexical_cast<string>(2*i), x));
    clustering_->push(one);
    one.clear();

    vector<core::clustering::indexed_point> two;
    two.push_back(single_indexed_point(lexical_cast<string>(2*i+1), y));
    clustering_->push(two);
    two.clear();
  }

  clustering_->do_clustering();
  {
    if (method_ == "dbscan") {
      ASSERT_THROW(clustering_->get_k_center(), common::unsupported_method);
    } else {
      vector<datum> result = clustering_->get_k_center();
      ASSERT_EQ(std::size_t(conf_.k), result.size());
    }
  }

  datum x;
  x.num_values_.push_back(make_pair("a", 100));
  x.num_values_.push_back(make_pair("b", 1000));
  if (method_ == "dbscan") {
    ASSERT_THROW(clustering_->
                 get_nearest_members_light(x), common::unsupported_method);
    return;
  }

  core::clustering::index_cluster_unit result =
      clustering_->get_nearest_members_light(x);
  // std::cout << " ["; //debug out
  for (size_t i = 0; i < result.size(); ++i) {
      // std::cout
      //   << result[i].first << " => "
      //   << result[i].second << ", ";
      ASSERT_EQ((lexical_cast<int>(result[i].second)%2), 0);
  }
  // std::cout << "]" << std::endl;
}

TEST_P(clustering_test, get_core_members_light) {
  jubatus::util::math::random::mtrand r(0);
  int data_num = 50;

  for (int i = 0; i < data_num; ++i) {
    datum x, y;

    x.num_values_.push_back(make_pair("a", 100 + r.next_gaussian() * 20));
    x.num_values_.push_back(make_pair("b", 100 + r.next_gaussian() * 40));
    y.num_values_.push_back(make_pair("c", -1000 - r.next_gaussian() * 20));
    y.num_values_.push_back(make_pair("d", -1000 - r.next_gaussian() * 40));

    vector<core::clustering::indexed_point> one;
    one.push_back(single_indexed_point(lexical_cast<string>(2*i), x));
    clustering_->push(one);
    one.clear();

    vector<core::clustering::indexed_point> two;
    two.push_back(single_indexed_point(lexical_cast<string>(2*i+1), y));
    clustering_->push(two);
    two.clear();
  }

  clustering_->do_clustering();

  core::clustering::index_cluster_set result =
      clustering_->get_core_members_light();
  for (size_t i = 0; i < result.size(); ++i) {
    std::cout << i << " :[";  //  debug out
    if (lexical_cast<int>(result[i][0].second)%2 == 0) {
      for (size_t j = 0; j < result[i].size(); ++j)  {
        ASSERT_EQ(0, lexical_cast<int>(result[i][0].second)%2);
        std::cout << result[i][j].second << ", ";
      }
         } else {
      for (size_t j = 0; j < result[i].size(); ++j)  {
        ASSERT_EQ(1, lexical_cast<int>(result[i][0].second)%2);
        std::cout << result[i][j].second << ", ";
      }
    }
    std::cout << "]" << std::endl;
  }
  ASSERT_EQ(std::size_t(conf_.k), result.size());
  ASSERT_EQ(std::size_t(data_num), result[0].size());
  ASSERT_EQ(std::size_t(data_num), result[1].size());
}

TEST_P(clustering_test, get_nearest_center) {
  jubatus::util::math::random::mtrand r(0);

  for (int i = 0; i < conf_.bucket_size * 2; ++i) {
    datum x, y;
    x.num_values_.push_back(make_pair("a", 10000 + r.next_gaussian() * 20));
    x.num_values_.push_back(make_pair("b", 1000 + r.next_gaussian() * 400));
    y.num_values_.push_back(make_pair("c", -500000 - r.next_gaussian() * 100));
    y.num_values_.push_back(make_pair("d", -10000 - r.next_gaussian() * 50));

    vector<core::clustering::indexed_point> one;
    one.push_back(single_indexed_point(lexical_cast<string>(2*i), x));
    clustering_->push(one);
    one.clear();

    vector<core::clustering::indexed_point> two;
    two.push_back(single_indexed_point(lexical_cast<string>(2*i+1), y));
    clustering_->push(two);
    two.clear();
  }

  clustering_->do_clustering();

  {
    if (method_ == "dbscan") {
      ASSERT_THROW(clustering_->get_k_center(), common::unsupported_method);
      return;
    }
    vector<datum> result = clustering_->get_k_center();
    ASSERT_EQ(std::size_t(conf_.k), result.size());
  }

  for (int i = 0; i < 100; ++i) {
    {
      datum x;
      x.num_values_.push_back(make_pair("a", 10000 + r.next_gaussian() * 20));
      x.num_values_.push_back(make_pair("b", 1000 + r.next_gaussian() * 400));
      datum expect_near_x = clustering_->get_nearest_center(x);
      const vector<pair<string, double> >& result = expect_near_x.num_values_;

      map<string, double> point;
      for (size_t j = 0; j < result.size(); ++j) {
        point.insert(result[j]);
      }
      // Check if the result is belonging to the same cluster.
      // Difference of the value and the mean of its distribution are expected
      // to be lesser than 3-sigma.
      ASSERT_GT(20  * 3, std::abs(point["a"] - 10000));
      ASSERT_GT(400 * 3, std::abs(point["b"] - 1000));

      // center position sometimes includes a bit "c" or "d" component
      ASSERT_GT(100, std::abs(point["c"]));
      ASSERT_GT(100, std::abs(point["d"]));
      ASSERT_GE(4u, point.size());
    }
  }
}

TEST_P(clustering_test, empty_mix) {
  framework::linear_mixable* clustering_mixable =
    dynamic_cast<framework::linear_mixable*>(clustering_->get_mixable());
  shared_ptr<driver::clustering> other = create_driver();
  framework::linear_mixable* other_mixable =
    dynamic_cast<framework::linear_mixable*>(other->get_mixable());
  ASSERT_TRUE(clustering_mixable);
  ASSERT_TRUE(other_mixable);

  msgpack::sbuffer data;
  {
    core::framework::stream_writer<msgpack::sbuffer> st(data);
    core::framework::jubatus_packer jp(st);
    core::framework::packer pk(jp);
    clustering_mixable->get_diff(pk);
  }
  {
    msgpack::sbuffer sbuf;
    core::framework::stream_writer<msgpack::sbuffer> st(sbuf);
    core::framework::jubatus_packer jp(st);
    core::framework::packer pk(jp);
    other_mixable->get_diff(pk);

    msgpack::unpacked msg;
    msgpack::unpack(&msg, sbuf.data(), sbuf.size());
    framework::diff_object diff = other_mixable->convert_diff_object(msg.get());

    msgpack::unpacked data_msg;
    msgpack::unpack(&data_msg, data.data(), data.size());

    other_mixable->mix(data_msg.get(), diff);
    other_mixable->put_diff(diff);
  }
}

vector<pair<string, string> > parameter_list() {
  vector<pair<string, string> > ret;
  ret.push_back(make_pair("simple", "kmeans"));
  ret.push_back(make_pair("compressive", "kmeans"));
  ret.push_back(make_pair("simple", "dbscan"));
#ifdef JUBATUS_USE_EIGEN
  ret.push_back(make_pair("simple", "gmm"));
  ret.push_back(make_pair("compressive", "gmm"));
#endif
  return ret;
}

INSTANTIATE_TEST_CASE_P(clustering_test_instance,
                        clustering_test,
                        testing::ValuesIn(parameter_list()));



class clustering_with_idf_test
    : public ::testing::TestWithParam<pair<string, string> > {
 protected:
  shared_ptr<driver::clustering> create_driver() const {
    pair<string, string> param = GetParam();
    common::jsonconfig::config config;
    common::jsonconfig::config compressor_config;
    if (param.first == "simple") {
      compressor_config = make_simple_config_idf();
    } else {
      compressor_config = make_compressive_config_idf();
    }

    if (param.second == "kmeans") {
      config = make_kmeans_config();
    } else if (param.second == "gmm") {
      config = make_gmm_config();
    } else {
      config = make_dbscan_config();
    }
    return shared_ptr<driver::clustering>(new driver::clustering(
               core::clustering::clustering_factory::create(
                                                            "dummy",
                                                            param.second,
                                                            param.first,
                                                            config,
                                                            compressor_config),
               make_tf_idf_fv_converter()));
  }

  void SetUp() {
    pair<string, string> param = GetParam();
    clustering_config conf;
    conf.bucket_size = 50;
    conf.bicriteria_base_size = 5;
    conf.compressed_bucket_size = 10;
    clustering_ = create_driver();
    method_ = param.second;
  }
  void TearDown() {
    clustering_.reset();
  }
  shared_ptr<driver::clustering> clustering_;
  string method_;
};

fv_converter::datum create_datum_str(const string& key, const string& value) {
  fv_converter::datum d;
  d.string_values_.push_back(make_pair(key, value));
  return d;
}

TEST_P(clustering_with_idf_test, get_nearest_members) {
  jubatus::util::math::random::mtrand r(0);
  vector<core::clustering::indexed_point> one;
  vector<core::clustering::indexed_point> two;

  for (int i = 0; i < 200 ; ++i) {
    const string i_str = lexical_cast<string>(i);
    one.push_back(single_indexed_point(
        lexical_cast<string>(i),
        create_datum_str("a" + i_str, "x y z " + i_str)));
    two.push_back(single_indexed_point(
        lexical_cast<string>(i),
        create_datum_str("a" + i_str, "y z w " + i_str)));
  }
  clustering_->push(one);
  clustering_->push(two);

  clustering_->do_clustering();

  {
    if (method_ == "dbscan") {
      ASSERT_THROW(clustering_->get_k_center(), common::unsupported_method);
      return;
    }
    vector<datum> result = clustering_->get_k_center();
    ASSERT_EQ(2u, result.size());
  }
}

INSTANTIATE_TEST_CASE_P(clustering_with_idf_test_instance,
                        clustering_with_idf_test,
                        testing::ValuesIn(parameter_list()));

}  // namespace driver
}  // namespace core
}  // namespace jubatus

