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

#include <limits>
#include <string>
#include <map>
#include <utility>

#include <gtest/gtest.h>

#include "clustering.hpp"
#include "clustering_method_factory.hpp"
#include "storage_factory.hpp"
#include "../common/jsonconfig.hpp"

using std::map;
using std::string;
using jubatus::util::text::json::json;
using jubatus::util::text::json::json_object;
using jubatus::util::text::json::to_json;
using jubatus::util::lang::shared_ptr;

namespace jubatus {
namespace core {
namespace clustering {

common::jsonconfig::config make_simple_config() {
  json js(new json_object);
  js = new json_object;
  js["bucket_size"] = to_json(200);
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
  js["eps"] = to_json(2.0);
  js["min_core_point"] = to_json(1);
  common::jsonconfig::config conf(js);
  return conf;
}

class make_case_type {
 public:
  make_case_type& operator()(const string& key, const string& value) {
    cases_.insert(make_pair(key, value));
    return *this;
  }

  map<string, string> operator()() {
    map<string, string> ret;
    ret.swap(cases_);
    return ret;
  }

 private:
  map<string, string> cases_;
} make_case;

class clustering_test
    : public ::testing::TestWithParam<map<string, string> > {
};

TEST_P(clustering_test, clustering_tests) {
  string n("name");
  map<string, string> param = GetParam();
  string m = param["method"];

  common::jsonconfig::config method_config;
  common::jsonconfig::config storage_config;

  if (param["method"] == "kmeans") {
    method_config = make_kmeans_config();
  } else if (param["method"] == "gmm") {
    method_config = make_gmm_config();
  } else if (param["method"] == "dbscan") {
    method_config = make_dbscan_config();
  } else {
    throw JUBATUS_EXCEPTION(common::unsupported_method(param["method"]));
  }

  if (param["compressor_method"] == "simple") {
    storage_config = make_simple_config();
  } else if (param["compressor_method"] == "compressive") {
    storage_config = make_compressive_config();
  } else {
    throw JUBATUS_EXCEPTION(
        common::unsupported_method(param["compressor_method"]));
  }

  ASSERT_NO_THROW(
      clustering k(
          clustering_method_factory::create(
               param["method"],
               method_config),
          storage_factory::create(
               n,
               param["method"],
               param["compressor_method"],
               storage_config)));
}

const map<string, string> test_cases[] = {
#ifdef JUBATUS_USE_EIGEN
  make_case("method", "gmm")
    ("compressor_method", "compressive")
    ("result", "false")(),
  make_case("method", "gmm")
    ("compressor_method", "compressive")
    ("result", "true")(),
  make_case("method", "gmm")
    ("compressor_method", "simple")
    ("result", "true")(),
#endif
  make_case("method", "kmeans")
    ("compressor_method", "compressive")
    ("result", "true")(),
  make_case("method", "kmeans")
    ("compressor_method", "compressive")
    ("result", "false")(),
  make_case("method", "kmeans")
    ("compressor_method", "simple")
    ("result", "true")(),
};

const map<string, string> test_cases_nocenter[] = {
  make_case("method", "dbscan")
  ("compressor_method", "simple")
  ("result", "true")(),
  make_case("method", "dbscan")
  ("compressor_method", "compressive")
  ("result", "false")()
};

INSTANTIATE_TEST_CASE_P(
    clustering_tests,
    clustering_test,
    ::testing::ValuesIn(test_cases));
}  // namespace clustering
}  // namespace core
}  // namespace jubatus
