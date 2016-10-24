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

#include <gtest/gtest.h>

#include "clustering_factory.hpp"
#include "../common/jsonconfig.hpp"

using jubatus::util::text::json::json;
using jubatus::util::text::json::json_null;
using jubatus::util::text::json::json_object;
using jubatus::util::text::json::to_json;

namespace jubatus {
namespace core {
namespace clustering {

TEST(clustering_factory_test, dbscan) {
  json js(new json_object);
  js["parameter"] = new json_object;
  js["parameter"]["eps"] = to_json(2.0);
  js["parameter"]["min_core_point"] = to_json(2);
  js["compressor_parameter"] = new json_object;
  js["compressor_parameter"]["bucket_size"] = to_json(10);
  common::jsonconfig::config conf(js);

  EXPECT_NO_THROW(clustering_factory::create(
                                             std::string("dbscan"),
                                             std::string("dbscan"),
                                             std::string("simple"),
                                             conf["parameter"],
                                             conf["compressor_parameter"]));

  // 0 < eps
  js["parameter"]["eps"] = to_json(0.0);
  EXPECT_THROW(clustering_factory::create(
                                          std::string("dbscan"),
                                          std::string("dbscan"),
                                          std::string("simple"),
                                          conf["parameter"],
                                          conf["compressor_parameter"]),
               common::invalid_parameter);
  js["parameter"]["eps"] = to_json(2.0);

  // 1 < min_core_point
  js["parameter"]["min_core_point"] = to_json(0);
  EXPECT_THROW(clustering_factory::create(
                                          std::string("dbscan"),
                                          std::string("dbscan"),
                                          std::string("simple"),
                                          conf["parameter"],
                                          conf["compressor_parameter"]),
               common::invalid_parameter);
  js["parameter"]["min_core_point"] = to_json(1);

  // dbscan supports only simple storage
  EXPECT_THROW(clustering_factory::create(
                                          std::string("dbscan"),
                                          std::string("dbscan"),
                                          std::string("compressive"),
                                          conf["parameter"],
                                          conf["compressor_parameter"]),
               common::unsupported_method);
}

TEST(clustering_factory_test, kmeans) {
  {
    json js(new json_object);
    js["parameter"] = new json_object;
    js["parameter"]["k"] = to_json(2);
    js["parameter"]["seed"] = to_json(2);
    js["compressor_parameter"] = new json_object;
    js["compressor_parameter"]["bucket_size"] = to_json(10);
    common::jsonconfig::config conf(js);

    EXPECT_NO_THROW(clustering_factory::create(
                                               std::string("kmeans"),
                                               std::string("kmeans"),
                                               std::string("simple"),
                                               conf["parameter"],
                                               conf["compressor_parameter"]));
    js["parameter"]["k"] = to_json(0);
    EXPECT_THROW(clustering_factory::create(
                                            std::string("kmeans"),
                                            std::string("kmeans"),
                                            std::string("simple"),
                                            conf["parameter"],
                                            conf["compressor_parameter"]),
                 common::invalid_parameter);
  }

  {
    json js(new json_object);
    js["parameter"] = new json_object;
    js["parameter"]["k"] = to_json(2);
    js["parameter"]["seed"] = to_json(2);
    js["compressor_parameter"] = new json_object;
    js["compressor_parameter"]["bucket_size"] = to_json(200);
    js["compressor_parameter"]["bucket_length"] = to_json(2);
    js["compressor_parameter"]["compressed_bucket_size"] = to_json(10);
    js["compressor_parameter"]["bicriteria_base_size"] = to_json(10);
    js["compressor_parameter"]["forgetting_factor"] = to_json(1.0);
    js["compressor_parameter"]["forgetting_threshold"] = to_json(0.05);
    js["compressor_parameter"]["seed"] = to_json(0);
    common::jsonconfig::config conf(js);

    EXPECT_NO_THROW(clustering_factory::create(
                                               std::string("kmeans"),
                                               std::string("kmeans"),
                                               std::string("compressive"),
                                               conf["parameter"],
                                               conf["compressor_parameter"]));

    // invalid compressor method
    EXPECT_THROW(clustering_factory::create(
                                            std::string("kmeans"),
                                            std::string("kmeans"),
                                            std::string("compress"),
                                            conf["parameter"],
                                            conf["compressor_parameter"]),
                 common::unsupported_method);

    // compressed_bucket_size <= bucket_size
    js["compressor_parameter"]["bucket_size"] = to_json(10);
    EXPECT_NO_THROW(clustering_factory::create(std::string("kmeans"),
                                               std::string("kmeans"),
                                               std::string("compressive"),
                                               conf["parameter"],
                                               conf["compressor_parameter"]));
    js["compressor_parameter"]["bucket_size"] = to_json(9);
    EXPECT_THROW(clustering_factory::create(
                                            std::string("kmeans"),
                                            std::string("kmeans"),
                                            std::string("compressive"),
                                            conf["parameter"],
                                            conf["compressor_parameter"]),
                 common::invalid_parameter);

    js["compressor_parameter"]["bucket_size"] = to_json(200);

    // 1 <= bicriteria_base_size
    js["compressor_parameter"]["bicriteria_base_size"] = to_json(0);
    EXPECT_THROW(clustering_factory::create(
                                            std::string("kmeans"),
                                            std::string("kmeans"),
                                            std::string("compressive"),
                                            conf["parameter"],
                                            conf["compressor_parameter"]),
                 common::invalid_parameter);

    // bicriteria_base_size <= compressed_bucket_size
    js["compressor_parameter"]["bicriteria_base_size"] = to_json(10);
    EXPECT_NO_THROW(clustering_factory::create(std::string("kmeans"),
                                               std::string("kmeans"),
                                               std::string("compressive"),
                                               conf["parameter"],
                                               conf["compressor_parameter"]));
    js["compressor_parameter"]["bicriteria_base_size"] = to_json(11);

    EXPECT_THROW(clustering_factory::create(
                                            std::string("kmeans"),
                                            std::string("kmeans"),
                                            std::string("compressive"),
                                            conf["parameter"],
                                            conf["compressor_parameter"]),
                 common::invalid_parameter);
  }
}

TEST(clustering_factory_test, gmm) {
  {
    json js(new json_object);
    js["parameter"] = new json_object;
    js["parameter"]["k"] = to_json(1);
    js["parameter"]["seed"] = to_json(2);
    js["compressor_parameter"] = new json_object;
    js["compressor_parameter"]["bucket_size"] = to_json(10);
    common::jsonconfig::config conf(js);

    EXPECT_NO_THROW(clustering_factory::create(
                                               std::string("gmm"),
                                               std::string("gmm"),
                                               std::string("simple"),
                                               conf["parameter"],
                                               conf["compressor_parameter"]));

    // 1 <= k
    js["parameter"]["k"] = to_json(0);
    EXPECT_THROW(clustering_factory::create(
                                            std::string("gmm"),
                                            std::string("gmm"),
                                            std::string("simple"),
                                            conf["parameter"],
                                            conf["compressor_parameter"]),
                 common::invalid_parameter);
  }

  {
    json js(new json_object);
    js["parameter"] = new json_object;
    js["parameter"]["k"] = to_json(2);
    js["parameter"]["seed"] = to_json(0);
    js["compressor_parameter"] = new json_object;
    js["compressor_parameter"]["bucket_size"] = to_json(2000);
    js["compressor_parameter"]["bucket_length"] = to_json(2);
    js["compressor_parameter"]["compressed_bucket_size"] = to_json(200);
    js["compressor_parameter"]["bicriteria_base_size"] = to_json(10);
    js["compressor_parameter"]["forgetting_factor"] = to_json(2);
    js["compressor_parameter"]["forgetting_threshold"] = to_json(0.05);
    js["compressor_parameter"]["seed"] = to_json(0);
    common::jsonconfig::config conf(js);

    EXPECT_NO_THROW(clustering_factory::create("gmm",
                                               "gmm",
                                               "compressive",
                                               conf["parameter"],
                                               conf["compressor_parameter"]));
  }
}
}
}
}
