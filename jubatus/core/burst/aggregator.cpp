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

    if (inputs_.empty()) {
      inputs_.push_front(
          make_new_window_(pos, input_window(0, batch_interval_, 0)));
    } else if (inputs_.front().get_end_pos() <= pos) {
      input_window new_window = make_new_window_(pos, inputs_[0]);
      inputs_.push_front(input_window());
      inputs_.front().swap(new_window);  // move semantics
      while (inputs_.size() > static_cast<size_t>(max_stored_)) {
        inputs_.pop_back();
      }
    }

    bool added = false;
    for (iterator_t iter = inputs_.begin(), end = inputs_.end();
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
    if (inputs_.empty()) {
      return 0;
    }

    burst_result prev = stored.get_result_at(
        inputs_.back().get_start_pos() - batch_interval_/2);

    typedef std::deque<input_window>::reverse_iterator iterator_t;

    for (iterator_t iter = inputs_.rbegin(), end = inputs_.rend();
         iter != end; ++iter) {
      burst_result r(*iter, scaling_param, gamma, costcut_threshold,
                     prev, max_reuse_batches);
      stored.store(r);
      prev.swap(r);  // more efficient than prev = r; use move when C++11/14
    }

    // erase inputs which will no longer be modified by add_document
    int n = 0;
    for (;;) {
      JUBATUS_ASSERT_GT(inputs_.size(), 0, "");

      std::pair<int, int> intersection =
          get_intersection(inputs_.back(), inputs_.front());
      if (intersection.first != intersection.second) {
        break;  // break if intersection exists
      }

      inputs_.pop_back();
      ++n;
    }
    // return erased count
    return n;
  }

  MSGPACK_DEFINE(inputs_, window_batch_size_, batch_interval_, max_stored_);

 private:
  std::deque<input_window> inputs_;
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

void aggregator::pack(framework::packer& packer) const {
  JUBATUS_ASSERT(p_);
  packer.pack(*p_);
}

void aggregator::unpack(msgpack::object o) {
  JUBATUS_ASSERT(p_);
  o.convert(p_.get());
}

}  // namespace burst
}  // namespace core
}  // namespace jubatus
