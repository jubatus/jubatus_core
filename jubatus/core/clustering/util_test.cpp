// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2015 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#include <cmath>
#include <gtest/gtest.h>
#include "../common/type.hpp"
#include "util.hpp"

using std::make_pair;
using jubatus::core::common::sfv_t;

namespace jubatus {
namespace core {
namespace clustering {

class util_test : public ::testing::Test {};


TEST_F(util_test, euclid_dist) {
  sfv_t p1;
  p1.push_back(make_pair("x", 1));
  p1.push_back(make_pair("y", 1));

  sfv_t p2;
  p2.push_back(make_pair("x", 2));
  p2.push_back(make_pair("y", 2));

  EXPECT_EQ(sfv_euclid_dist(p1, p1), 0);
  EXPECT_EQ(sfv_euclid_dist(p1, p2), std::sqrt(2));
}

TEST_F(util_test, cosine_test) {
  sfv_t p1;
  p1.push_back(make_pair("x", 0));
  p1.push_back(make_pair("y", 1));

  sfv_t p2;
  p2.push_back(make_pair("x", 1));
  p2.push_back(make_pair("y", 0));

  EXPECT_DOUBLE_EQ(0., sfv_cosine_dist(p1, p1));
  EXPECT_DOUBLE_EQ(1., sfv_cosine_dist(p1, p2));
}


}  // namespace clustering
}  // namespace core
}  // namespace jubatus
