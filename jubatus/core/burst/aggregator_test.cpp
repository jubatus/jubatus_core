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

#include "aggregator.hpp"

#include <cfloat>
#include <vector>
#include <gtest/gtest.h>

#include "result_storage.hpp"

namespace jubatus {
namespace core {
namespace burst {

inline int flush_results_with_default_params(
    aggregator& a, result_storage& r) {
  return a.flush_results(2, 1, DBL_MAX, 10, r);
}

TEST(aggregator, regular_input) {
  const int n = 100;
  const int batch_size = 10;
  const double batch_interval = 6.283;
  const double window_interval = batch_size * batch_interval;

  aggregator tested(batch_size, batch_interval, 2);
  result_storage results(2);

  for (int i = 0; i < n; ++i) {
    double start_pos = i * window_interval;
    for (int j = 0; j < batch_size; ++j) {
      double pos = start_pos + (j + 0.5) * batch_interval;

      // add test
      bool added = tested.add_document(1, 0, pos);
      ASSERT_TRUE(added);
      // a bit predated data (allowed)
      added = tested.add_document(1, 0, pos - batch_interval);
      ASSERT_TRUE(added);

      // out-of-range data
      ASSERT_FALSE(tested.add_document(1, 0, pos - window_interval));

      int m = flush_results_with_default_params(tested, results);
      if (m == 0) {
        if (i != 0) {
          ASSERT_NE(0, j);
          ASSERT_NE(batch_size/2, j);
        }
      } else {
        ASSERT_EQ(1, m);
        burst_result latest = results.get_latest_result();
        ASSERT_TRUE(j == 0 || j == batch_size/2);
        ASSERT_NEAR(start_pos + (j - batch_size/2) * batch_interval,
                    latest.get_end_pos(),
                    batch_interval / 100);
        const std::vector<batch_result>& batches = latest.get_batches();
        for (size_t k = 0; k < batches.size(); ++k) {
          double pos = latest.get_start_pos() + (k+0.5) * batch_interval;
          if (pos > 0) {
            ASSERT_EQ(2, batches[k].d);
            ASSERT_EQ(0, batches[k].r);
            ASSERT_EQ(0, batches[k].burst_weight);
          }
        }
      }
    }
  }
}

TEST(aggregator, sparse_input) {
  const int n = 100;
  const int batch_size = 10;
  const double batch_interval = 2.718;
  const double window_interval = batch_size * batch_interval;

  aggregator tested(batch_size, batch_interval, 2);
  result_storage results(2);

  double pos = (batch_size/2 + 0.5) * batch_interval, prev_pos = -1;
  for (int i = 0; i < n; ++i) {
    bool added = tested.add_document(1, 0, pos);
    ASSERT_TRUE(added);

    int m = flush_results_with_default_params(tested, results);
    if (prev_pos >= 0) {
      ASSERT_EQ(1, m);
      burst_result latest = results.get_latest_result();
      batch_result batch = latest.get_batch_at(prev_pos);
      ASSERT_EQ(1, batch.d);
      ASSERT_EQ(0, batch.r);
      ASSERT_EQ(0, batch.burst_weight);
    } else {
      ASSERT_EQ(0, m);
    }

    prev_pos = pos;
    pos += window_interval * 1.414;
  }
}

TEST(aggregator, lazy_flushing) {
  const int n = 10;
  const int batch_size = 10;
  const double batch_interval = 1.618;
  const double window_interval = batch_size * batch_interval;

  aggregator tested(batch_size, batch_interval, n);
  result_storage results(n);

  const double pos0 = (batch_size/2 + 0.5) * batch_interval;
  {
    double pos = pos0;
    for (int i = 0; i < n; ++i) {
      bool added = tested.add_document(1, 0, pos);
      ASSERT_TRUE(added);

      pos += window_interval * 1.732;
    }
  }

  int m = flush_results_with_default_params(tested, results);
  ASSERT_EQ(n-1, m);

  {
    double pos = pos0;
    for (int i = 0; i < n-1; ++i) {
      burst_result result = results.get_result_at(pos);
      batch_result batch = result.get_batch_at(pos);
      ASSERT_EQ(1, batch.d);
      ASSERT_EQ(0, batch.r);
      ASSERT_EQ(0, batch.burst_weight);

      pos += window_interval * 1.732;
    }
  }
}

TEST(aggregator, lazy_flushing_overflow) {
  const int n = 10;
  const int k = 5;
  const int batch_size = 10;
  const double batch_interval = 1.618;
  const double window_interval = batch_size * batch_interval;

  aggregator tested(batch_size, batch_interval, k);
  result_storage results(k);

  const double pos0 = (batch_size/2 + 0.5) * batch_interval;
  {
    double pos = pos0;
    for (int i = 0; i < n; ++i) {
      bool added = tested.add_document(1, 0, pos);
      ASSERT_TRUE(added);

      pos += window_interval * 1.732;
    }
  }

  int m = flush_results_with_default_params(tested, results);
  ASSERT_EQ(k, m);

  {
    double pos = pos0;
    for (int i = 0; i < n-1; ++i) {
      burst_result result = results.get_result_at(pos);
      if (i < n-k-1) {
        ASSERT_FALSE(result.is_valid());
      } else {
        batch_result batch = result.get_batch_at(pos);
        ASSERT_EQ(1, batch.d);
        ASSERT_EQ(0, batch.r);
        ASSERT_EQ(0, batch.burst_weight);
      }

      pos += window_interval * 1.732;
    }
  }
}

}  // namespace burst
}  // namespace core
}  // namespace jubatus
