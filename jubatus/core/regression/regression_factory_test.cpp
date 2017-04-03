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

#include <stdexcept>
#include <string>

#include <gtest/gtest.h>
#include "jubatus/util/text/json.h"

#include "regression_factory.hpp"
#include "regression.hpp"
#include "../storage/local_storage.hpp"
#include "../common/exception.hpp"
#include "../common/jsonconfig.hpp"

using jubatus::util::lang::shared_ptr;
using jubatus::util::text::json::json;
using jubatus::util::text::json::to_json;
using jubatus::util::text::json::json_object;

namespace jubatus {
namespace core {
namespace regression {

TEST(regression_factory, trivial) {
  regression::regression_factory f;
  shared_ptr<storage::local_storage> s(new storage::local_storage);

  {
    common::jsonconfig::config param(to_json(
      regression::passive_aggressive::config()));
    shared_ptr<regression::regression_base> r =
      f.create_regression("PA", param, s);
    EXPECT_EQ(typeid(*r), typeid(regression::passive_aggressive&));
  }

  {
    common::jsonconfig::config param(to_json(
      regression::passive_aggressive_1::config()));
    shared_ptr<regression::regression_base> r =
      f.create_regression("PA1", param, s);
    EXPECT_EQ(typeid(*r), typeid(regression::passive_aggressive_1&));
  }

  {
    common::jsonconfig::config param(to_json(
      regression::passive_aggressive_2::config()));
    shared_ptr<regression::regression_base> r =
      f.create_regression("PA2", param, s);
    EXPECT_EQ(typeid(*r), typeid(regression::passive_aggressive_2&));
  }

  {
    common::jsonconfig::config param(to_json(
      regression::perceptron::config()));
    shared_ptr<regression::regression_base> r =
      f.create_regression("perceptron", param, s);
    EXPECT_EQ(typeid(*r), typeid(regression::perceptron&));
  }

  {
    common::jsonconfig::config param(to_json(
      regression::confidence_weighted::config()));
    shared_ptr<regression::regression_base> r =
      f.create_regression("CW", param, s);
    EXPECT_EQ(typeid(*r), typeid(regression::confidence_weighted&));
  }

  {
    common::jsonconfig::config param(to_json(
      regression::arow::config()));
    shared_ptr<regression::regression_base> r =
      f.create_regression("AROW", param, s);
    EXPECT_EQ(typeid(*r), typeid(regression::arow&));
  }

  {
    common::jsonconfig::config param(to_json(
      regression::normal_herd::config()));
    shared_ptr<regression::regression_base> r =
      f.create_regression("NHERD", param, s);
    EXPECT_EQ(typeid(*r), typeid(regression::normal_herd&));
  }

  {
    json js(new json_object);
    js["method"] = to_json(std::string("lsh"));
    js["parameter"] = json(new json_object);
    js["parameter"]["hash_num"] = to_json(8);
    js["nearest_neighbor_num"] = to_json(5);
    js["weight"] = to_json(std::string("distance"));
    common::jsonconfig::config param(js);
    shared_ptr<regression::regression_base> r =
      f.create_regression("NN", param, s);
    EXPECT_EQ(typeid(*r), typeid(regression::nearest_neighbor_regression&));
  }

  {
    common::jsonconfig::config param(to_json(
        regression::inverted_index_regression::config()));
    shared_ptr<regression::regression_base> r =
      f.create_regression("euclidean", param, s);
    EXPECT_EQ(typeid(*r), typeid(regression::euclidean_distance_regression&));
  }

  {
    common::jsonconfig::config param(to_json(
        regression::inverted_index_regression::config()));
    shared_ptr<regression::regression_base> r =
      f.create_regression("cosine", param, s);
    EXPECT_EQ(typeid(*r), typeid(regression::cosine_similarity_regression&));
  }
}

TEST(regression_factory, unknown) {
  regression::regression_factory f;
  shared_ptr<storage::local_storage> s(new storage::local_storage);
  common::jsonconfig::config param;
  ASSERT_THROW(f.create_regression("unknown_regression", param, s),
               common::unsupported_method);
}

}  // namespace regression
}  // namespace core
}  // namespace jubatus
