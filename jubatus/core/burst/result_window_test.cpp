// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2014 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

TEST(default_constructed, result_window) {
  result_window w;

  ASSERT_DOUBLE_EQ(0, w.get_start_pos());
  ASSERT_DOUBLE_EQ(0, w.get_end_pos());
  ASSERT_DOUBLE_EQ(0, w.get_all_interval());
  ASSERT_EQ(0, w.get_batch_size());
  ASSERT_LT(0, w.get_batch_interval());
  ASSERT_EQ(0u, w.get_batches().size());
}

struct result_window_test_params {
  double start_pos;
  double batch_interval;
  int n;

  result_window_test_params(double start_pos_, double interval_, int n_)
      : start_pos(start_pos_), batch_interval(interval_), n(n_) {
  }
};

class result_window_test
    : public ::testing::TestWithParam<result_window_test_params> {
 protected:
  void SetUp() {
    start_pos = GetParam().start_pos;
    n = GetParam().n;
    batch_interval = GetParam().batch_interval;

    input = input_window(start_pos, batch_interval, n);
    weights.resize(n);

    // TODO(gintenlabo): fill input and weights randomly

    tested = result_window(input, weights);
  }

  input_window input;
  std::vector<double> weights;
  result_window tested;
  double start_pos;
  int n;
  double batch_interval;
};

TEST_P(result_window_test, basics) {
  ASSERT_DOUBLE_EQ(start_pos, tested.get_start_pos());
  ASSERT_DOUBLE_EQ(start_pos + n*batch_interval, tested.get_end_pos());
  ASSERT_DOUBLE_EQ(n*batch_interval, tested.get_all_interval());
  ASSERT_EQ(n, tested.get_batch_size());
  ASSERT_EQ(batch_interval, tested.get_batch_interval());

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

INSTANTIATE_TEST_CASE_P(result_window_test_, result_window_test,
    ::testing::Values(result_window_test_params(2, 10, 10),
                      result_window_test_params(-2, 1.1, 1),
                      result_window_test_params(0, 5, 0)));

}  // namespace burst
}  // namespace core
}  // namespace jubatus
