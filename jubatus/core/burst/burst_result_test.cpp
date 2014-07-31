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

#include "burst_result.hpp"

#include <climits>
#include <vector>
#include <gtest/gtest.h>

#include "input_window.hpp"
#include "result_window.hpp"
#include "../framework/stream_writer.hpp"
#include "jubatus/util/math/random.h"

using jubatus::util::math::random::mtrand;

namespace jubatus {
namespace core {
namespace burst {

inline burst_result make_result_with_default_params(
    const input_window& input,
    const burst_result& prev_result,
    int max_reuse) {
  return burst_result(input, 2, 1, DBL_MAX, prev_result, max_reuse);
}

inline burst_result make_burst_result_randomly(
    double start_pos, double batch_interval, int batch_size,
    int data_count, double relevant_rate, mtrand& rand) {
  input_window input(start_pos, batch_interval, batch_size);
  double end_pos = input.get_end_pos();

  for (int i = 0; i < data_count; ++i) {
    int d = 1;
    int r = rand.next_double() < relevant_rate ? 1 : 0;
    double pos = rand.next_double(start_pos, end_pos);

    input.add_document(d, r, pos);
  }
  return make_result_with_default_params(input, burst_result(), 0);
}


TEST(burst_result, default_ctor) {
  burst_result r;

  EXPECT_FALSE(r.is_valid());

  EXPECT_EQ(burst_result::invalid_pos, r.get_start_pos());
  EXPECT_EQ(burst_result::invalid_pos, r.get_end_pos());
  EXPECT_EQ(0, r.get_all_interval());
  EXPECT_EQ(0, r.get_batch_size());
  EXPECT_EQ(0u, r.get_batches().size());
  EXPECT_FALSE(r.is_bursted_at_latest_batch());
}

TEST(burst_result, simple) {
  const double start_pos = 1;
  const double batch_interval = 1;
  const int n = 5;

  input_window input(start_pos, batch_interval, n);

  for (int i = 0; i < n; ++i) {
    double pos = start_pos + (i + 0.5) * batch_interval;
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
    double pos = start_pos + (i + 0.5) * batch_interval;
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
  const double batch_interval = 1;
  const int n = 5;

  input_window input(start_pos, batch_interval, n);

  for (int i = 0; i < n; ++i) {
    double pos = start_pos + (i + 0.5) * batch_interval;
    if (i == 0 || i == 4) {
      input.add_document(10, 10, pos);
    } else {
      input.add_document(10, 0, pos);
    }
  }

  input_window prev_input(start_pos - batch_interval*3, batch_interval, 5);
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

TEST(burst_result, has_same_batch_interval) {
  burst_result r0, r1(result_window(1, 1)), r2(result_window(1, 2));
  burst_result r1_(result_window(2, 1.0001));

  // expected behavior
  EXPECT_FALSE(r2.has_same_batch_interval(r1));
  EXPECT_FALSE(r1.has_same_batch_interval(r2));
  EXPECT_TRUE(r1.has_same_batch_interval(r1_));
  EXPECT_TRUE(r1_.has_same_batch_interval(r1));

  // always false when rhs and/or lhs is default constructed
  EXPECT_FALSE(r0.has_same_batch_interval(r0));
  EXPECT_FALSE(r0.has_same_batch_interval(r1));
  EXPECT_FALSE(r2.has_same_batch_interval(r0));

  // reflexive
  EXPECT_TRUE(r1.has_same_batch_interval(r1));
  EXPECT_TRUE(r2.has_same_batch_interval(r2));
}

inline ::testing::AssertionResult burst_result_equals_to(
    const burst_result& x, const burst_result& y) {
  if (!x.has_same_start_pos(y)) {
    return ::testing::AssertionFailure() << "start_pos mismatched: "
        << x.get_start_pos() << " vs " << y.get_start_pos();
  }
  if (!x.has_same_batch_interval(y)) {
    return ::testing::AssertionFailure() << "batch_interval mismatched: "
        << x.get_batch_interval() << " vs " << y.get_batch_interval();
  }

  const std::vector<batch_result>& batches1 = x.get_batches();
  const std::vector<batch_result>& batches2 = y.get_batches();
  if (batches1.size() != batches2.size()) {
    return ::testing::AssertionFailure() << "batch_size mismatched: "
        << x.get_batch_size() << " vs " << y.get_batch_size();
  }
  for (size_t i = 0; i < batches1.size(); ++i) {
    const batch_result& b1 = batches1[i];
    const batch_result& b2 = batches2[i];
    if (b1.d != b2.d || b1.r != b2.r ||
        std::abs(b1.burst_weight - b2.burst_weight)
            > std::abs(b1.burst_weight) * 0.001) {
      return ::testing::AssertionFailure() << i << "th batch mismatched: "
          << "(" << b1.d << "," << b1.r << "," << b1.burst_weight << ") vs "
          << "(" << b2.d << "," << b2.r << "," << b2.burst_weight << ")";
    }
  }
  return ::testing::AssertionSuccess();
}

TEST(burst_result, pack_and_unpack) {
  mtrand rand(testing::UnitTest::GetInstance()->random_seed());

  burst_result a = make_burst_result_randomly(3, 1, 10, 100, 1.0/10, rand);

  msgpack::sbuffer buf;
  framework::stream_writer<msgpack::sbuffer> st(buf);
  framework::jubatus_packer jp(st);
  framework::packer packer(jp);
  packer.pack(a);

  burst_result b;
  {
    msgpack::unpacked unpacked;
    msgpack::unpack(&unpacked, buf.data(), buf.size());
    unpacked.get().convert(&b);
  }

  ASSERT_TRUE(burst_result_equals_to(a, b));
}

}  // namespace burst
}  // namespace core
}  // namespace jubatus
