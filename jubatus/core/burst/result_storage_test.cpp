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

#include "result_storage.hpp"

#include <vector>
#include <gtest/gtest.h>

#include "input_window.hpp"
#include "result_window.hpp"

namespace jubatus {
namespace core {
namespace burst {

inline burst_result make_result(double start_pos, double batch_length, int n) {
  input_window input(start_pos, batch_length, n);
  std::vector<double> weights(n, -1);
  return burst_result(result_window(input, weights));
}

TEST(result_storage, general) {
  const int n = 5;
  const double batch_length = 1;

  result_storage storage(n);

  storage.store(make_result(0, batch_length, 10));
  ASSERT_DOUBLE_EQ(0, storage.get_latest_result().get_start_pos());
  ASSERT_DOUBLE_EQ(0, storage.get_result_at(batch_length/2).get_start_pos());

  for (int i = 1; i < n + 1; ++i) {
    double pos = batch_length * i;
    storage.store(make_result(pos, batch_length, 10));
    ASSERT_DOUBLE_EQ(pos, storage.get_latest_result().get_start_pos());
    ASSERT_DOUBLE_EQ(pos,
        storage.get_result_at(pos + batch_length/2).get_start_pos());
    ASSERT_DOUBLE_EQ(batch_length * (i-1),
        storage.get_result_at(pos - batch_length/2).get_start_pos());
  }

  // first stored result should be erased
  ASSERT_DOUBLE_EQ(burst_result::invalid_pos,
      storage.get_result_at(batch_length/2).get_start_pos());
}

TEST(result_storage, unsequenced) {
  const int n = 5;
  const double batch_length = 1;

  result_storage storage(n);

  storage.store(make_result(0, batch_length, 10));
  storage.store(make_result(1, batch_length, 10));
  storage.store(make_result(2, batch_length, 10));
  storage.store(make_result(3, batch_length, 10));
  storage.store(make_result(2, batch_length, 10));
  storage.store(make_result(3, batch_length, 10));
  storage.store(make_result(4, batch_length, 10));
  storage.store(make_result(3, batch_length, 10));
  storage.store(make_result(2, batch_length, 10));
  storage.store(make_result(1, batch_length, 10));

  // first stored result should not be erased
  ASSERT_DOUBLE_EQ(0, storage.get_result_at(0.5).get_start_pos());

  // sequenced
  ASSERT_DOUBLE_EQ(1, storage.get_result_at(1.5).get_start_pos());
  ASSERT_DOUBLE_EQ(2, storage.get_result_at(2.5).get_start_pos());
  ASSERT_DOUBLE_EQ(3, storage.get_result_at(3.5).get_start_pos());
  ASSERT_DOUBLE_EQ(4, storage.get_result_at(4.5).get_start_pos());
}

}  // namespace burst
}  // namespace core
}  // namespace jubatus
