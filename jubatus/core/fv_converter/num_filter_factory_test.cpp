// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2011 Preferred Infrastructure and Nippon Telegraph and Telephone Corporation.
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

#include <map>
#include <string>
#include <gtest/gtest.h>
#include "../common/exception.hpp"
#include "exception.hpp"
#include "num_filter_factory.hpp"
#include "num_filter_impl.hpp"

namespace jubatus {
namespace core {
namespace fv_converter {

TEST(num_filter_factory, unknown_name) {
  num_filter_factory f;
  EXPECT_THROW(f.create("unknonw", std::map<std::string, std::string>()),
      converter_exception);
}

TEST(num_filter_factory, create_add_filter_with_illegal_config) {
  num_filter_factory f;
  std::map<std::string, std::string> params;
  EXPECT_THROW(f.create("add", params), converter_exception);
  params["value"] = "hoge";
  EXPECT_THROW(f.create("add", params), std::bad_cast);
}

TEST(num_filter_factory, create_add_filter) {
  num_filter_factory f;

  std::map<std::string, std::string> params;
  params["value"] = "10";
  jubatus::util::lang::shared_ptr<num_filter> filter(f.create("add", params));

  EXPECT_EQ(typeid(add_filter).name(), typeid(*filter).name());
  EXPECT_EQ(20.0, filter->filter(10.0));
}

TEST(num_filter_factory, create_linear_normalization_filter) {
  num_filter_factory f;
  std::map<std::string, std::string> params;
  params["min"] = "-100";
  params["max"] = "100";
  jubatus::util::lang::shared_ptr<num_filter>
      filter(f.create("linear_normalization", params));

  EXPECT_EQ(typeid(linear_normalization_filter), typeid(*filter));
  EXPECT_DOUBLE_EQ(0, filter->filter(-100));
  EXPECT_DOUBLE_EQ(0.25, filter->filter(-50));
  EXPECT_DOUBLE_EQ(0.5, filter->filter(0));
  EXPECT_DOUBLE_EQ(0.75, filter->filter(50));
  EXPECT_DOUBLE_EQ(1, filter->filter(100));
}

TEST(num_filter_factory, create_illegal_linear_normalization_filter) {
  num_filter_factory f;
  std::map<std::string, std::string> params;
  params["min"] = "100";
  params["max"] = "100";
  EXPECT_THROW(f.create("linear_normalization", params),
               common::invalid_parameter);

  params["min"] = "100";
  params["max"] = "99.99";
  EXPECT_THROW(f.create("linear_normalization", params),
               common::invalid_parameter);
}

TEST(num_filter_factory, create_gaussian_normalization_filter) {
  num_filter_factory f;
  std::map<std::string, std::string> params;
  params["average"] = "100";
  params["standard_deviation"] = "100";
  jubatus::util::lang::shared_ptr<num_filter>
      filter(f.create("gaussian_normalization", params));

  EXPECT_EQ(typeid(gaussian_normalization_filter),
            typeid(*filter));
  EXPECT_DOUBLE_EQ(-1, filter->filter(0));
  EXPECT_DOUBLE_EQ(-0.5, filter->filter(50));
  EXPECT_DOUBLE_EQ(0, filter->filter(100));
  EXPECT_DOUBLE_EQ(0.5, filter->filter(150));
  EXPECT_DOUBLE_EQ(1, filter->filter(200));
}

TEST(num_filter_factory, exception_with_minus_standard_deviation) {
  num_filter_factory f;
  std::map<std::string, std::string> params;
  params["average"] = "100";
  params["standard_deviation"] = "-1";
  EXPECT_THROW(f.create("gaussian_normalization", params),
               common::invalid_parameter);
}

TEST(num_filter_factory, create_sigmoid_normalization_filter) {
  num_filter_factory f;
  std::map<std::string, std::string> params;
  params["gain"] = "1";
  params["bias"] = "0";
  jubatus::util::lang::shared_ptr<num_filter>
      filter(f.create("sigmoid_normalization", params));

  EXPECT_EQ(typeid(sigmoid_normalization_filter),
            typeid(*filter));
  EXPECT_DOUBLE_EQ(0.5, filter->filter(0));
  EXPECT_DOUBLE_EQ(1, filter->filter(999999999999999999));
  EXPECT_DOUBLE_EQ(0, filter->filter(-999999999999999999));
}

}  // namespace fv_converter
}  // namespace core
}  // namespace jubatus
