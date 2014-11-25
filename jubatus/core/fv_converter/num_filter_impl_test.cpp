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

#include <gtest/gtest.h>
#include "num_filter_impl.hpp"

namespace jubatus {
namespace core {
namespace fv_converter {

TEST(add_filter, trivial) {
  add_filter add(1.0);
  EXPECT_EQ(3.0, add.filter(2.0));
}

TEST(linear_normalization, truncate) {
  linear_normalization_filter truncate_normalizer(-100, 100, true);
  for (int i = 0; i < 1000; ++i) {
    // cut to zero
    EXPECT_DOUBLE_EQ(0, truncate_normalizer.filter(-100 - i * 100));
  }
  EXPECT_DOUBLE_EQ(0, truncate_normalizer.filter(-100));
  EXPECT_DOUBLE_EQ(0.25, truncate_normalizer.filter(-50));
  EXPECT_DOUBLE_EQ(0.5, truncate_normalizer.filter(0));
  EXPECT_DOUBLE_EQ(0.75, truncate_normalizer.filter(50));
  EXPECT_DOUBLE_EQ(1, truncate_normalizer.filter(100));
  for (int i = 0; i < 1000; ++i) {
    // cut to one
    EXPECT_DOUBLE_EQ(1, truncate_normalizer.filter(100 + i * 100));
  }
}

TEST(linear_normalization, non_truncate) {
  linear_normalization_filter truncate_normalizer(-100, 100, false);
  for (int i = 0; i < 1000; ++i) {
    EXPECT_DOUBLE_EQ(-0.5 * i, truncate_normalizer.filter(-100 + i * -100));
  }
  EXPECT_DOUBLE_EQ(0, truncate_normalizer.filter(-100));
  EXPECT_DOUBLE_EQ(0.25, truncate_normalizer.filter(-50));
  EXPECT_DOUBLE_EQ(0.5, truncate_normalizer.filter(0));
  EXPECT_DOUBLE_EQ(0.75, truncate_normalizer.filter(50));
  EXPECT_DOUBLE_EQ(1, truncate_normalizer.filter(100));
  EXPECT_DOUBLE_EQ(1.5, truncate_normalizer.filter(200));
  for (int i = 0; i < 1000; ++i) {
    EXPECT_DOUBLE_EQ(1.0 + 0.5 * i, truncate_normalizer.filter(100 + i * 100));
  }
}

TEST(gaussian_normalization, negative_standard_deviation) {
  EXPECT_THROW(gaussian_normalization_filter a(1, -1),
               common::invalid_parameter);
}

TEST(gaussian_normalization, trivial) {
  gaussian_normalization_filter gaussian_normalizer(100, 100);
  EXPECT_DOUBLE_EQ(0, gaussian_normalizer.filter(100));
  EXPECT_DOUBLE_EQ(-0.5, gaussian_normalizer.filter(50));
  EXPECT_DOUBLE_EQ(-1, gaussian_normalizer.filter(-0));
  EXPECT_DOUBLE_EQ(0.5, gaussian_normalizer.filter(150));
  EXPECT_DOUBLE_EQ(1, gaussian_normalizer.filter(200));
}

TEST(sigmoid_normalization, trivial) {
  sigmoid_normalization_filter sigmoid_normalizer(1, 0);
  EXPECT_DOUBLE_EQ(0.5, sigmoid_normalizer.filter(0));
  EXPECT_DOUBLE_EQ(1, sigmoid_normalizer.filter(99999999999999));
  EXPECT_DOUBLE_EQ(0, sigmoid_normalizer.filter(-9999999999999999));
}

TEST(sigmoid_normalization, biased) {
  sigmoid_normalization_filter sigmoid_normalizer(1, 10);
  EXPECT_DOUBLE_EQ(0.5, sigmoid_normalizer.filter(10));
  EXPECT_DOUBLE_EQ(1, sigmoid_normalizer.filter(99999999999999));
  EXPECT_DOUBLE_EQ(0, sigmoid_normalizer.filter(-9999999999999999));
}

}  // namespace fv_converter
}  // namespace core
}  // namespace jubatus
