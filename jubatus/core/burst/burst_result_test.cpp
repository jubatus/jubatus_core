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
#include "result_window.hpp"

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

  input_window input(start_pos, batch_length, n);

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

  input_window input(start_pos, batch_length, n);

  for (int i = 0; i < n; ++i) {
    double pos = start_pos + (i + 0.5) * batch_length;
    if (i == 0 || i == 4) {
      input.add_document(10, 10, pos);
    } else {
      input.add_document(10, 0, pos);
    }
  }

  input_window prev_input(start_pos - batch_length*3, batch_length, 5);
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

TEST(burst_result, is_older_than) {
  burst_result r0, r1(result_window(1, 1)), r2(result_window(2, 1));
  burst_result r1_(result_window(1.0001, 1));

  // expected behavior
  EXPECT_TRUE(r1.is_older_than(r2));
  EXPECT_FALSE(r2.is_older_than(r1));
  EXPECT_FALSE(r1.is_older_than(r1_));
  EXPECT_FALSE(r1_.is_older_than(r1));

  // always false when rhs and/or lhs is default constructed
  EXPECT_FALSE(r0.is_older_than(r0));
  EXPECT_FALSE(r0.is_older_than(r1));
  EXPECT_FALSE(r2.is_older_than(r0));

  // irreflexive
  EXPECT_FALSE(r1.is_older_than(r1));
  EXPECT_FALSE(r2.is_older_than(r2));
}

TEST(burst_result, is_newer_than) {
  burst_result r0, r1(result_window(1, 1)), r2(result_window(2, 1));
  burst_result r1_(result_window(1.0001, 1));

  // expected behavior
  EXPECT_TRUE(r2.is_newer_than(r1));
  EXPECT_FALSE(r1.is_newer_than(r2));
  EXPECT_FALSE(r1.is_newer_than(r1_));
  EXPECT_FALSE(r1_.is_newer_than(r1));

  // always false when rhs and/or lhs is default constructed
  EXPECT_FALSE(r0.is_newer_than(r0));
  EXPECT_FALSE(r0.is_newer_than(r1));
  EXPECT_FALSE(r2.is_newer_than(r0));

  // irreflexive
  EXPECT_FALSE(r1.is_newer_than(r1));
  EXPECT_FALSE(r2.is_newer_than(r2));
}

TEST(burst_result, has_same_start_pos) {
  burst_result r0, r1(result_window(1, 1)), r2(result_window(2, 1));
  burst_result r1_(result_window(1.0001, 2));

  // expected behavior
  EXPECT_FALSE(r2.has_same_start_pos(r1));
  EXPECT_FALSE(r1.has_same_start_pos(r2));
  EXPECT_TRUE(r1.has_same_start_pos(r1_));
  EXPECT_TRUE(r1_.has_same_start_pos(r1));

  // always false when rhs and/or lhs is default constructed
  EXPECT_FALSE(r0.has_same_start_pos(r0));
  EXPECT_FALSE(r0.has_same_start_pos(r1));
  EXPECT_FALSE(r2.has_same_start_pos(r0));

  // reflexive
  EXPECT_TRUE(r1.has_same_start_pos(r1));
  EXPECT_TRUE(r2.has_same_start_pos(r2));
}

TEST(burst_result, has_same_batch_length) {
  burst_result r0, r1(result_window(1, 1)), r2(result_window(1, 2));
  burst_result r1_(result_window(2, 1.0001));

  // expected behavior
  EXPECT_FALSE(r2.has_same_batch_length(r1));
  EXPECT_FALSE(r1.has_same_batch_length(r2));
  EXPECT_TRUE(r1.has_same_batch_length(r1_));
  EXPECT_TRUE(r1_.has_same_batch_length(r1));

  // always false when rhs and/or lhs is default constructed
  EXPECT_FALSE(r0.has_same_batch_length(r0));
  EXPECT_FALSE(r0.has_same_batch_length(r1));
  EXPECT_FALSE(r2.has_same_batch_length(r0));

  // reflexive
  EXPECT_TRUE(r1.has_same_batch_length(r1));
  EXPECT_TRUE(r2.has_same_batch_length(r2));
}

}  // namespace burst
}  // namespace core
}  // namespace jubatus
