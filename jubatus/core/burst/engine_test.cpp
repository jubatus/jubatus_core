// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2014 Preferred Infrastructure and Nippon Telegraph and Telephone Corporation.
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

#include "engine.hpp"

#include <climits>
#include <vector>
#include <gtest/gtest.h>

namespace jubatus {
namespace core {
namespace burst {

inline void burst_detect_with_default_params(
    const std::vector<uint32_t>& d_vec,
    const std::vector<uint32_t>& r_vec,
    std::vector<double>& weights) {
  burst_detect(d_vec, r_vec, weights, 2, 1, DBL_MAX);
}

TEST(engine, trivial) {
  const int n = 5;

  std::vector<uint32_t> d_vec, r_vec;
  std::vector<double> weights;

  d_vec.resize(n, 10);
  r_vec.resize(n, 0);
  weights.resize(n, -1);

  burst_detect_with_default_params(d_vec, r_vec, weights);

  for (size_t i = 0; i < weights.size(); ++i) {
    EXPECT_EQ(0, weights[i]);
  }
}

TEST(engine, simple) {
  const int n = 5;

  std::vector<uint32_t> d_vec, r_vec;
  std::vector<double> weights;

  d_vec.resize(n, 10);
  r_vec.resize(n, 0);
  weights.resize(n, -1);

  r_vec[0] = 10;
  r_vec[4] = 10;

  burst_detect_with_default_params(d_vec, r_vec, weights);

  EXPECT_LT(6, weights[0]);
  EXPECT_EQ(0, weights[1]);
  EXPECT_EQ(0, weights[2]);
  EXPECT_EQ(0, weights[3]);
  EXPECT_LT(6, weights[4]);
}

TEST(engine, reuse) {
  const int n = 5;

  std::vector<uint32_t> d_vec, r_vec;
  std::vector<double> weights;

  d_vec.resize(n, 10);
  r_vec.resize(n, 0);
  weights.resize(n, -1);

  r_vec[0] = 10;
  r_vec[4] = 10;

  weights[0] = 0;
  weights[1] = 0;

  burst_detect_with_default_params(d_vec, r_vec, weights);

  EXPECT_EQ(0, weights[0]);  // masked by reuse
  EXPECT_EQ(0, weights[1]);
  EXPECT_EQ(0, weights[2]);
  EXPECT_EQ(0, weights[3]);
  EXPECT_LT(6, weights[4]);
}

}  // namespace burst
}  // namespace core
}  // namespace jubatus
