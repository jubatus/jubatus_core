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

#include "aggregator.hpp"

#include <utility>
#include <deque>
#include <vector>

#include "../common/assert.hpp"
#include "input_window.hpp"
#include "window_intersection.hpp"

namespace jubatus {
namespace core {
namespace burst {

class aggregator::impl_ {
 public:
  impl_(int window_batch_size, double batch_interval, int max_stored)
      : window_batch_size_(window_batch_size),
        batch_interval_(batch_interval),
        max_stored_(max_stored) {
  }

  bool add_document(int d, int r, double pos) {
    typedef std::deque<input_window>::iterator iterator_t;

    if (aggregating_.empty()) {
      aggregating_.push_front(
          make_new_window_(pos, input_window(0, batch_interval_, 0)));
    } else if (aggregating_[0].get_end_pos() <= pos) {
      input_window new_window = make_new_window_(pos, aggregating_[0]);
      aggregating_.push_front(input_window());
      aggregating_.front().swap(new_window);  // move semantics
      move_aggregated_inputs_();
    }

    bool added = false;
    for (iterator_t iter = aggregating_.begin(), end = aggregating_.end();
         iter != end; ++iter) {
      if (iter->add_document(d, r, pos)) {
        added = true;
      } else {
        return added;
      }
    }
    return added;
  }

  int flush_results(double scaling_param,
                    double gamma,
                    double costcut_threshold,
                    int max_reuse_batches,
                    result_storage& stored) {
    const int n = aggregated_.size();

    if (n == 0) {
      return 0;
    }

    typedef std::deque<input_window>::iterator iterator_t;

    burst_result prev = stored.get_latest_result();
    for (iterator_t iter = aggregated_.begin(), end = aggregated_.end();
         iter != end; ++iter) {
      burst_result r(*iter, scaling_param, gamma, costcut_threshold,
                     prev, max_reuse_batches);
      stored.store(r);
      prev.swap(r);  // more efficient than prev = r; use move when C++11/14
    }

    // clear aggregated
    std::deque<input_window>().swap(aggregated_);

    return n;
  }

 private:
  std::deque<input_window> aggregating_, aggregated_;
  int window_batch_size_;
  double batch_interval_;
  int max_stored_;

  input_window make_new_window_(double pos, const input_window& prev) const {
    double prev_start_pos = prev.get_start_pos();
    int i = static_cast<int>(
              std::floor((pos - prev_start_pos) / batch_interval_));
    int j = i - window_batch_size_/2;
    double new_start_pos = prev_start_pos + batch_interval_ * j;

    input_window new_window(
        new_start_pos, batch_interval_, window_batch_size_);

    // fill new_window's d&r vector
    std::pair<int, int> intersection = get_intersection(prev, new_window);
    for (int i = 0, j = intersection.first;
         j < intersection.second;
         ++i, ++j) {
      JUBATUS_ASSERT_LT(i, window_batch_size_, "");
      new_window.get_batch_by_index(i) = prev.get_batch_by_index(j);
    }

    return new_window;  // NRVO
  }

  void move_aggregated_inputs_() {
    for (;;) {
      std::pair<int, int> intersection =
          get_intersection(aggregating_.back(), aggregating_.front());
      if (intersection.first != intersection.second) {
        break;
      }
      JUBATUS_ASSERT_GT(aggregating_.size(), 1, "");

      aggregated_.push_back(input_window());
      aggregated_.back().swap(aggregating_.back());  // move semantics
      aggregating_.pop_back();

      while (aggregated_.size() > static_cast<size_t>(max_stored_)) {
        aggregated_.pop_front();
      }
    }
  }
};

aggregator::aggregator(
    int window_batch_size, double batch_interval, int max_stored)
    : p_(new impl_(window_batch_size, batch_interval, max_stored)) {
}

aggregator::~aggregator() {
}

bool aggregator::add_document(int d, int r, double pos) {
  JUBATUS_ASSERT(p_);
  return p_->add_document(d, r, pos);
}

int aggregator::flush_results(double scaling_param,
                              double gamma,
                              double costcut_threshold,
                              int max_reuse_batches,
                              result_storage& stored) {
  JUBATUS_ASSERT(p_);
  return p_->flush_results(scaling_param, gamma, costcut_threshold,
                           max_reuse_batches, stored);
}

}  // namespace burst
}  // namespace core
}  // namespace jubatus
