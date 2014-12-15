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

#include "window_intersection.hpp"

#include <algorithm>
#include <vector>
#include <utility>
#include <gtest/gtest.h>

namespace jubatus {
namespace core {
namespace burst {

// helper functions
::testing::AssertionResult check_index_for_boundary(
    int expected, const intersection_helper& w, double pos) {
  int actual = w.get_index_for_boundary(pos);
  if (actual == expected) {
    return ::testing::AssertionSuccess() << "expected: " << expected
                << "; actual: " << actual;
  } else {
    return ::testing::AssertionFailure() << "expected: " << expected
                << "; actual: " << actual;
  }
}
::testing::AssertionResult check_batch_interval(
    double interval, const intersection_helper& w) {
  if (w.has_batch_interval_equals_to(interval)) {
    return ::testing::AssertionSuccess() << "expected: " << interval;
  } else {
    return ::testing::AssertionFailure() << "expected: " << interval;
  }
}

struct window_intersection_test_params {
  double start_pos;
  double batch_interval;
  int n;

  window_intersection_test_params(double start_pos_, double interval_, int n_)
      : start_pos(start_pos_), batch_interval(interval_), n(n_) {
  }
};

typedef basic_window<int> sample_window;

class window_intersection_test
    : public ::testing::TestWithParam<window_intersection_test_params> {
 protected:
  void SetUp() {
    start_pos = GetParam().start_pos;
    n = GetParam().n;
    batch_interval = GetParam().batch_interval;
    w0 = sample_window(start_pos, batch_interval, n);
    tested = intersection_helper(w0);
  }

  sample_window w0;
  intersection_helper tested;
  double start_pos;
  int n;
  double batch_interval;
};

TEST_P(window_intersection_test, get_index_for_boundary) {
  double eps = batch_interval * 0.01 / 2;

  #define CHECK_INDEX_FOR_BOUNDARY(expected, pos)                         \
    do {                                                                  \
      double pos_ = pos;                                                  \
      ASSERT_TRUE(check_index_for_boundary(expected, tested, pos_));      \
      ASSERT_TRUE(check_index_for_boundary(expected, tested, pos_-eps));  \
      ASSERT_TRUE(check_index_for_boundary(expected, tested, pos_-eps));  \
      ASSERT_TRUE(check_index_for_boundary(tested.no_index(),             \
                                               tested, pos_ + 10*eps));   \
      ASSERT_TRUE(check_index_for_boundary(tested.no_index(),             \
                                               tested, pos_ - 10*eps));   \
    } while (0)
  // CHECK_INDEX_FOR_BOUNDARY

    for (int i = -5; i <= n+5; ++i) {
      double pos = start_pos + i * batch_interval;
      CHECK_INDEX_FOR_BOUNDARY(i, pos);
      CHECK_INDEX_FOR_BOUNDARY(tested.no_index(), pos + batch_interval / 2);
    }

  #undef CHECK_INDEX_FOR_BOUNDARY
}

TEST_P(window_intersection_test, has_batch_interval_equals_to) {
  double eps = batch_interval * 0.01 / 2;
  double eps_n = eps / (n > 0 ? n : 1);

  ASSERT_TRUE(check_batch_interval(batch_interval, tested));
  ASSERT_TRUE(check_batch_interval(batch_interval - eps_n, tested));
  ASSERT_TRUE(check_batch_interval(batch_interval + eps_n, tested));
  ASSERT_FALSE(check_batch_interval(batch_interval - 10*eps_n, tested));
  ASSERT_FALSE(check_batch_interval(batch_interval + 10*eps_n, tested));
}

TEST_P(window_intersection_test, intersect_to_empty_window) {
  sample_window w;
  std::pair<int, int> intersection = get_intersection(w0, w);
  ASSERT_EQ(intersection.first, intersection.second);
}

TEST_P(window_intersection_test, intersect_to_totally_left1) {
  sample_window w(start_pos - batch_interval*2, batch_interval, 1);
  std::pair<int, int> intersection = get_intersection(w0, w);
  ASSERT_EQ(0, intersection.first);
  ASSERT_EQ(0, intersection.second);
}

TEST_P(window_intersection_test, intersect_to_totally_left2) {
  sample_window w(start_pos - batch_interval*1.5, batch_interval, 1);
  std::pair<int, int> intersection = get_intersection(w0, w);
  ASSERT_EQ(tested.no_index(), intersection.first);
  ASSERT_EQ(tested.no_index(), intersection.second);
}

TEST_P(window_intersection_test, intersect_to_partially_left0) {
  sample_window w(start_pos - batch_interval*2, batch_interval, 2);
  std::pair<int, int> intersection = get_intersection(w0, w);
  ASSERT_EQ(0, intersection.first);
  ASSERT_EQ(0, intersection.second);
}

TEST_P(window_intersection_test, intersect_to_partially_left1) {
  sample_window w(start_pos - batch_interval*1, batch_interval, 4);
  std::pair<int, int> intersection = get_intersection(w0, w);
  ASSERT_EQ(0, intersection.first);
  ASSERT_EQ(std::min(n, 3), intersection.second);
}

TEST_P(window_intersection_test, intersect_to_partially_left2) {
  sample_window w(start_pos - batch_interval*0.5, batch_interval, 4);
  std::pair<int, int> intersection = get_intersection(w0, w);
  ASSERT_EQ(tested.no_index(), intersection.first);
  ASSERT_EQ(tested.no_index(), intersection.second);
}

TEST_P(window_intersection_test, intersect_to_overlap0) {
  sample_window w(start_pos, batch_interval, n);
  std::pair<int, int> intersection = get_intersection(w0, w);
  ASSERT_EQ(0, intersection.first);
  ASSERT_EQ(n, intersection.second);
}

TEST_P(window_intersection_test, intersect_to_overlap1) {
  sample_window w(start_pos-batch_interval, batch_interval, n+2);
  std::pair<int, int> intersection = get_intersection(w0, w);
  ASSERT_EQ(0, intersection.first);
  ASSERT_EQ(n, intersection.second);
}

TEST_P(window_intersection_test, intersect_to_overlap2) {
  int i = std::min(2, n);
  int j = std::max(i, n-1);
  ASSERT_LE(i, j);
  sample_window w(start_pos+batch_interval*i, batch_interval, j-i);
  std::pair<int, int> intersection = get_intersection(w0, w);
  ASSERT_EQ(i, intersection.first);
  ASSERT_EQ(j, intersection.second);
}

TEST_P(window_intersection_test, intersect_to_overlap3) {
  int i = std::min(2, n);
  int j = std::max(i, n-1);
  ASSERT_LE(i, j);
  sample_window w(start_pos+batch_interval*(i-0.5), batch_interval, j-i);
  std::pair<int, int> intersection = get_intersection(w0, w);
  ASSERT_EQ(tested.no_index(), intersection.first);
  ASSERT_EQ(tested.no_index(), intersection.second);
}

TEST_P(window_intersection_test, intersect_to_partially_right0) {
  double end_pos = w0.get_end_pos();
  sample_window w(end_pos, batch_interval, 2);
  std::pair<int, int> intersection = get_intersection(w0, w);
  ASSERT_EQ(n, intersection.first);
  ASSERT_EQ(n, intersection.second);
}

TEST_P(window_intersection_test, intersect_to_partially_right1) {
  double end_pos = w0.get_end_pos();
  sample_window w(end_pos - batch_interval*1, batch_interval, 4);
  std::pair<int, int> intersection = get_intersection(w0, w);
  ASSERT_EQ(std::max(0, n-1), intersection.first);
  ASSERT_EQ(n, intersection.second);
}

TEST_P(window_intersection_test, intersect_to_partially_right2) {
  double end_pos = w0.get_end_pos();
  sample_window w(end_pos - batch_interval*0.5, batch_interval, 4);
  std::pair<int, int> intersection = get_intersection(w0, w);
  ASSERT_EQ(tested.no_index(), intersection.first);
  ASSERT_EQ(tested.no_index(), intersection.second);
}

TEST_P(window_intersection_test, intersect_to_totally_right1) {
  double end_pos = w0.get_end_pos();
  sample_window w(end_pos + batch_interval*2, batch_interval, 1);
  std::pair<int, int> intersection = get_intersection(w0, w);
  ASSERT_EQ(n, intersection.first);
  ASSERT_EQ(n, intersection.second);
}

TEST_P(window_intersection_test, intersect_to_totally_right2) {
  double end_pos = w0.get_end_pos();
  sample_window w(end_pos + batch_interval*1.5, batch_interval, 1);
  std::pair<int, int> intersection = get_intersection(w0, w);
  ASSERT_EQ(tested.no_index(), intersection.first);
  ASSERT_EQ(tested.no_index(), intersection.second);
}

INSTANTIATE_TEST_CASE_P(window_intersection_test_, window_intersection_test,
    ::testing::Values(window_intersection_test_params(2, 10, 10),
                      window_intersection_test_params(-2, 1.1, 1),
                      window_intersection_test_params(0, 5, 0)));

}  // namespace burst
}  // namespace core
}  // namespace jubatus
