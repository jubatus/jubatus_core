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

#include "burst_result.hpp"

#include <climits>
#include <vector>
#include <gtest/gtest.h>

#include "input_window.hpp"

namespace jubatus {
namespace core {
namespace burst {

inline burst_result make_result_with_default_params(
    const input_window& input,
    const burst_result& prev_result,
    int max_reuse) {
  return burst_result(input, 2, 1, DBL_MAX, prev_result, max_reuse);
}

TEST(burst_result, default_ctor) {
  burst_result r;

  EXPECT_FALSE(r.is_valid());

  EXPECT_EQ(burst_result::invalid_pos, r.get_start_pos());
  EXPECT_EQ(burst_result::invalid_pos, r.get_end_pos());
  EXPECT_EQ(0, r.get_all_length());
  EXPECT_EQ(0, r.get_batch_size());
  EXPECT_EQ(0u, r.get_batches().size());
  EXPECT_FALSE(r.is_bursted_at_latest_batch());
}

TEST(burst_result, simple) {
  const double start_pos = 1;
  const double batch_length = 1;
  const int n = 5;

  input_window input(start_pos, batch_length*n, n);

  for (int i = 0; i < n; ++i) {
    double pos = start_pos + (i + 0.5) * batch_length;
    if (i == 0 || i == 4) {
      input.add_document(10, 10, pos);
    } else {
      input.add_document(10, 0, pos);
    }
  }

  burst_result r = make_result_with_default_params(input, burst_result(), n);
  ASSERT_TRUE(r.is_valid());

  const std::vector<batch_result>& results = r.get_batches();
  ASSERT_EQ(5u, results.size());
  EXPECT_LT(6, results[0].burst_weight);
  EXPECT_EQ(0, results[1].burst_weight);
  EXPECT_EQ(0, results[2].burst_weight);
  EXPECT_EQ(0, results[3].burst_weight);
  EXPECT_LT(6, results[4].burst_weight);

  // check is_bursted_at
  for (int i = 0; i < n; ++i) {
    double pos = start_pos + (i + 0.5) * batch_length;
    if (i == 0 || i == 4) {
      EXPECT_TRUE(r.is_bursted_at(pos));
    } else {
      EXPECT_FALSE(r.is_bursted_at(pos));
    }
  }
  EXPECT_TRUE(r.is_bursted_at_latest_batch());
}

TEST(burst_result, reuse) {
  const double start_pos = 1;
  const double batch_length = 1;
  const int n = 5;

  input_window input(start_pos, batch_length*n, n);

  for (int i = 0; i < n; ++i) {
    double pos = start_pos + (i + 0.5) * batch_length;
    if (i == 0 || i == 4) {
      input.add_document(10, 10, pos);
    } else {
      input.add_document(10, 0, pos);
    }
  }

  input_window prev_input(start_pos - batch_length*3, batch_length*5, 5);
  burst_result prev_result =
      make_result_with_default_params(prev_input, burst_result(), 0);

  burst_result r = make_result_with_default_params(input, prev_result, n);
  ASSERT_TRUE(r.is_valid());

  const std::vector<batch_result>& results = r.get_batches();
  ASSERT_EQ(5u, results.size());
  EXPECT_EQ(0, results[0].burst_weight);  // masked bu reuse
  EXPECT_EQ(0, results[1].burst_weight);
  EXPECT_EQ(0, results[2].burst_weight);
  EXPECT_EQ(0, results[3].burst_weight);
  EXPECT_LT(6, results[4].burst_weight);
}

}  // namespace burst
}  // namespace core
}  // namespace jubatus
