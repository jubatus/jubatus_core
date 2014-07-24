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

#include "result_window.hpp"

#include <algorithm>
#include <vector>
#include <utility>
#include <gtest/gtest.h>

namespace jubatus {
namespace core {
namespace burst {

// helper functions
::testing::AssertionResult check_index_for_intersection(
    int expected, const result_window& w, double pos) {
  int actual = w.get_index_for_intersection(pos);
  if (actual == expected) {
    return ::testing::AssertionSuccess() << "expected: " << expected
                << "; actual: " << actual;
  } else {
    return ::testing::AssertionFailure() << "expected: " << expected
                << "; actual: " << actual;
  }
}
::testing::AssertionResult check_batch_length(
    double len, const result_window& w) {
  if (w.has_batch_length_equals_to(len)) {
    return ::testing::AssertionSuccess() << "expected: " << len
                << "; actual: " << w.get_batch_length();
  } else {
    return ::testing::AssertionFailure() << "expected: " << len
                << "; actual: " << w.get_batch_length();
  }
}

TEST(default_constructed, result_window) {
  result_window w;

  ASSERT_DOUBLE_EQ(0, w.get_start_pos());
  ASSERT_DOUBLE_EQ(0, w.get_end_pos());
  ASSERT_DOUBLE_EQ(0, w.get_all_length());
  ASSERT_EQ(0, w.get_batch_size());
  ASSERT_LT(0, w.get_batch_length());
  ASSERT_EQ(0u, w.get_batches().size());

  for (int i = -5; i <= 5; ++i) {
    double pos = w.get_start_pos() + i * w.get_batch_length();
    ASSERT_TRUE(check_index_for_intersection(i, w, pos));
  }
}

struct result_window_test_params {
  double start_pos;
  double batch_length;
  int n;

  result_window_test_params(double start_pos_, double len_, int n_)
      : start_pos(start_pos_), batch_length(len_), n(n_) {
  }
};

class result_window_test
    : public ::testing::TestWithParam<result_window_test_params> {
 protected:
  void SetUp() {
    start_pos = GetParam().start_pos;
    n = GetParam().n;
    batch_length = GetParam().batch_length;

    input = input_window(start_pos, batch_length, n);
    weights.resize(n);

    // TODO(gintenlabo): fill input and weights randomly

    tested = result_window(input, weights);
  }

  input_window input;
  std::vector<double> weights;
  result_window tested;
  double start_pos;
  int n;
  double batch_length;
};

TEST_P(result_window_test, basics) {
  ASSERT_DOUBLE_EQ(start_pos, tested.get_start_pos());
  ASSERT_DOUBLE_EQ(start_pos + n*batch_length, tested.get_end_pos());
  ASSERT_DOUBLE_EQ(n*batch_length, tested.get_all_length());
  ASSERT_EQ(n, tested.get_batch_size());
  ASSERT_EQ(batch_length, tested.get_batch_length());

  std::vector<batch_input> const& inputs = input.get_batches();
  std::vector<batch_result> const& results = tested.get_batches();

  ASSERT_EQ(static_cast<size_t>(n), inputs.size());
  ASSERT_EQ(static_cast<size_t>(n), weights.size());
  ASSERT_EQ(static_cast<size_t>(n), results.size());

  for (int i = 0; i < n; ++i) {
    ASSERT_EQ(inputs[i].d, results[i].d);
    ASSERT_EQ(inputs[i].r, results[i].r);
    ASSERT_DOUBLE_EQ(weights[i], results[i].burst_weight);
  }
}

TEST_P(result_window_test, get_index_for_intersection) {
  double eps = batch_length * 0.01 / 2;

  #define CHECK_INDEX_FOR_INTERSECTION(expected, pos)                        \
    do {                                                                     \
      double pos_ = pos;                                                     \
      ASSERT_TRUE(check_index_for_intersection(expected, tested, pos_));     \
      ASSERT_TRUE(check_index_for_intersection(expected, tested, pos_-eps)); \
      ASSERT_TRUE(check_index_for_intersection(expected, tested, pos_-eps)); \
      ASSERT_TRUE(check_index_for_intersection(tested.no_index(),            \
                                               tested, pos_ + 10*eps));      \
      ASSERT_TRUE(check_index_for_intersection(tested.no_index(),            \
                                               tested, pos_ - 10*eps));      \
    } while (0)
  // CHECK_INDEX_FOR_INTERSECTION

    for (int i = -5; i <= n+5; ++i) {
      double pos = start_pos + i * batch_length;
      CHECK_INDEX_FOR_INTERSECTION(i, pos);
      CHECK_INDEX_FOR_INTERSECTION(tested.no_index(), pos + batch_length / 2);
    }

  #undef CHECK_INDEX_FOR_INTERSECTION
}

TEST_P(result_window_test, has_batch_length_equals_to) {
  double eps = batch_length * 0.01 / 2;
  double eps_n = eps / (n > 0 ? n : 1);

  ASSERT_TRUE(check_batch_length(batch_length, tested));
  ASSERT_TRUE(check_batch_length(batch_length - eps_n, tested));
  ASSERT_TRUE(check_batch_length(batch_length + eps_n, tested));
  ASSERT_FALSE(check_batch_length(batch_length - 10*eps_n, tested));
  ASSERT_FALSE(check_batch_length(batch_length + 10*eps_n, tested));
}

typedef basic_window<int> sample_window;

TEST_P(result_window_test, intersect_to_empty_window) {
  sample_window w;
  std::pair<int, int> intersection = tested.get_intersection(w);
  ASSERT_EQ(intersection.first, intersection.second);
}

TEST_P(result_window_test, intersect_to_totally_left1) {
  sample_window w(start_pos - batch_length*2, batch_length, 1);
  std::pair<int, int> intersection = tested.get_intersection(w);
  ASSERT_EQ(0, intersection.first);
  ASSERT_EQ(0, intersection.second);
}

TEST_P(result_window_test, intersect_to_totally_left2) {
  sample_window w(start_pos - batch_length*1.5, batch_length, 1);
  std::pair<int, int> intersection = tested.get_intersection(w);
  ASSERT_EQ(tested.no_index(), intersection.first);
  ASSERT_EQ(tested.no_index(), intersection.second);
}

TEST_P(result_window_test, intersect_to_partially_left0) {
  sample_window w(start_pos - batch_length*2, batch_length, 2);
  std::pair<int, int> intersection = tested.get_intersection(w);
  ASSERT_EQ(0, intersection.first);
  ASSERT_EQ(0, intersection.second);
}

TEST_P(result_window_test, intersect_to_partially_left1) {
  sample_window w(start_pos - batch_length*1, batch_length, 4);
  std::pair<int, int> intersection = tested.get_intersection(w);
  ASSERT_EQ(0, intersection.first);
  ASSERT_EQ(std::min(n, 3), intersection.second);
}

TEST_P(result_window_test, intersect_to_partially_left2) {
  sample_window w(start_pos - batch_length*0.5, batch_length, 4);
  std::pair<int, int> intersection = tested.get_intersection(w);
  ASSERT_EQ(tested.no_index(), intersection.first);
  ASSERT_EQ(tested.no_index(), intersection.second);
}

TEST_P(result_window_test, intersect_to_overlap0) {
  sample_window w(start_pos, batch_length, n);
  std::pair<int, int> intersection = tested.get_intersection(w);
  ASSERT_EQ(0, intersection.first);
  ASSERT_EQ(n, intersection.second);
}

TEST_P(result_window_test, intersect_to_overlap1) {
  sample_window w(start_pos-batch_length, batch_length, n+2);
  std::pair<int, int> intersection = tested.get_intersection(w);
  ASSERT_EQ(0, intersection.first);
  ASSERT_EQ(n, intersection.second);
}

TEST_P(result_window_test, intersect_to_overlap2) {
  int i = std::min(2, n);
  int j = std::max(i, n-1);
  ASSERT_LE(i, j);
  sample_window w(start_pos+batch_length*i, batch_length, j-i);
  std::pair<int, int> intersection = tested.get_intersection(w);
  ASSERT_EQ(i, intersection.first);
  ASSERT_EQ(j, intersection.second);
}

TEST_P(result_window_test, intersect_to_overlap3) {
  int i = std::min(2, n);
  int j = std::max(i, n-1);
  ASSERT_LE(i, j);
  sample_window w(start_pos+batch_length*(i-0.5), batch_length, j-i);
  std::pair<int, int> intersection = tested.get_intersection(w);
  ASSERT_EQ(tested.no_index(), intersection.first);
  ASSERT_EQ(tested.no_index(), intersection.second);
}

TEST_P(result_window_test, intersect_to_partially_right0) {
  double end_pos = tested.get_end_pos();
  sample_window w(end_pos, batch_length, 2);
  std::pair<int, int> intersection = tested.get_intersection(w);
  ASSERT_EQ(n, intersection.first);
  ASSERT_EQ(n, intersection.second);
}

TEST_P(result_window_test, intersect_to_partially_right1) {
  double end_pos = tested.get_end_pos();
  sample_window w(end_pos - batch_length*1, batch_length, 4);
  std::pair<int, int> intersection = tested.get_intersection(w);
  ASSERT_EQ(std::max(0, n-1), intersection.first);
  ASSERT_EQ(n, intersection.second);
}

TEST_P(result_window_test, intersect_to_partially_right2) {
  double end_pos = tested.get_end_pos();
  sample_window w(end_pos - batch_length*0.5, batch_length, 4);
  std::pair<int, int> intersection = tested.get_intersection(w);
  ASSERT_EQ(tested.no_index(), intersection.first);
  ASSERT_EQ(tested.no_index(), intersection.second);
}

TEST_P(result_window_test, intersect_to_totally_right1) {
  double end_pos = tested.get_end_pos();
  sample_window w(end_pos + batch_length*2, batch_length, 1);
  std::pair<int, int> intersection = tested.get_intersection(w);
  ASSERT_EQ(n, intersection.first);
  ASSERT_EQ(n, intersection.second);
}

TEST_P(result_window_test, intersect_to_totally_right2) {
  double end_pos = tested.get_end_pos();
  sample_window w(end_pos + batch_length*1.5, batch_length, 1);
  std::pair<int, int> intersection = tested.get_intersection(w);
  ASSERT_EQ(tested.no_index(), intersection.first);
  ASSERT_EQ(tested.no_index(), intersection.second);
}

INSTANTIATE_TEST_CASE_P(result_window_test_, result_window_test,
    ::testing::Values(result_window_test_params(2, 10, 10),
                      result_window_test_params(-2, 1.1, 1),
                      result_window_test_params(0, 5, 0)));

}  // namespace burst
}  // namespace core
}  // namespace jubatus
