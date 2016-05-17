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

#include "weight.hpp"
#include "../fv_converter/datum.hpp"

#include "test_util.hpp"

using std::string;
using std::vector;
using std::pair;
using std::make_pair;
using std::isfinite;
using std::numeric_limits;
using std::cout;
using std::endl;

using jubatus::util::lang::shared_ptr;
using jubatus::core::fv_converter::datum;

namespace jubatus {
namespace core {
namespace driver {

class weight_test : public ::testing::Test {
 protected:
  void SetUp() {
    weight_.reset(new driver::weight(make_tf_idf_fv_converter()));
  }

  void TearDown() {
    weight_.reset();
  }

  shared_ptr<core::driver::weight> weight_;
};

TEST_F(weight_test, simple) {
  common::sfv_t fv;

  // |D| = 1
  datum d1;
  d1.string_values_.push_back(make_pair("key1", "a1 b1 b1"));
  fv = weight_->update(d1);

  // The initial registration of IDF-weighted keys will be 0
  EXPECT_EQ(0, fv.size());

  // |D| = 2
  datum d2;
  d2.string_values_.push_back(make_pair("key2", "a1 a1"));
  fv = weight_->update(d2);

  ASSERT_EQ(1, fv.size());
  EXPECT_EQ("key2$a1@space#tf/idf", fv[0].first);
  EXPECT_FLOAT_EQ(2.0 * std::log((2.0 + 1) / (1.0 + 1)), fv[0].second);

  // |D| = 2
  fv = weight_->calc_weight(d1);
  ASSERT_EQ(2, fv.size());
  EXPECT_EQ("key1$a1@space#tf/idf", fv[0].first);
  EXPECT_FLOAT_EQ(std::log((2.0 + 1.0) / (1.0 + 1.0)), fv[0].second);
  EXPECT_EQ("key1$b1@space#tf/idf", fv[1].first);
  EXPECT_FLOAT_EQ(2.0 * std::log((2.0 + 1.0) / (1.0 + 1.0)), fv[1].second);
}

}  // driver namespace
}  // core namespace
}  // jubatus namespace
