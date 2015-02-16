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

#ifndef JUBATUS_CORE_BURST_INPUT_WINDOW_HPP_
#define JUBATUS_CORE_BURST_INPUT_WINDOW_HPP_

#include <stdint.h>
#include <algorithm>
#include <cmath>
#include <vector>
#include <msgpack.hpp>
#include "../common/assert.hpp"
#include "../common/exception.hpp"

#include "window_fwd.hpp"

namespace jubatus {
namespace core {
namespace burst {

template<class Batch>
class basic_window {
 public:
  typedef Batch batch_type;

  explicit basic_window(double batch_interval = 1)
      : batches_(), start_pos_(0), batch_interval_(batch_interval) {
    if (!(batch_interval > 0)) {
      throw JUBATUS_EXCEPTION(common::invalid_parameter(
          "window: batch_interval should be > 0"));
    }
  }

  basic_window(double start_pos, double batch_interval, int32_t batch_size)
      : batches_(), start_pos_(start_pos), batch_interval_(batch_interval) {
    if (!(batch_interval > 0)) {
      throw JUBATUS_EXCEPTION(common::invalid_parameter(
          "window: batch_interval should be > 0"));
    }
    if (!(batch_size >= 0)) {
      throw JUBATUS_EXCEPTION(common::invalid_parameter(
          "window: batch_size should be >= 0"));
    }
    batches_.resize(batch_size);
  }

  // get index for pos; return -1 if out of range
  int get_index(double pos) const {
    int n = get_batch_size();
    int i = get_index_(pos);

    if (i < 0 || n <= i) {
      JUBATUS_ASSERT(!contains(pos));
      return -1;
    }

    JUBATUS_ASSERT(contains(pos));
    return i;
  }

  double get_start_pos() const {
    return start_pos_;
  }
  double get_end_pos() const {
    return start_pos_ + get_all_interval();
  }
  bool contains(double pos) const {
    return get_start_pos() <= pos && pos < get_end_pos();
  }
  double get_all_interval() const {
    return batch_interval_ * get_batch_size();
  }

  int32_t get_batch_size() const {
    return batches_.size();
  }
  double get_batch_interval() const {
    return batch_interval_;
  }

  const std::vector<batch_type>& get_batches() const {
    return batches_;
  }

  batch_type& get_batch_by_index(size_t i) {
    return batches_[i];
  }
  const batch_type& get_batch_by_index(size_t i) const {
    return batches_[i];
  }

  void swap(basic_window& x) {
    using std::swap;
    swap(batches_, x.batches_);
    swap(start_pos_, x.start_pos_);
    swap(batch_interval_, x.batch_interval_);
  }
  friend void swap(basic_window& x, basic_window& y) {
    x.swap(y);
  }

 protected:
  std::vector<batch_type> batches_;
  double start_pos_;
  double batch_interval_;

  // get index for pos; no out of range, return as if extended
  int get_index_(double pos) const {
    return static_cast<int>(
        std::floor((pos - start_pos_) / get_batch_interval()));
  }
};

class input_window : private basic_window<batch_input> {
  typedef basic_window<batch_input> base_;

 public:
  explicit input_window(double start_pos = 0,
                        double batch_interval = 1,
                        int32_t batch_size = 0)
      : base_(start_pos, batch_interval, batch_size) {
  }

  bool add_document(uint32_t d, uint32_t r, double pos) {
    int i = base_::get_index(pos);

    if (i < 0) {
      return false;
    }

    batches_[i].d += d;
    batches_[i].r += r;
    return true;
  }

  using base_::get_start_pos;
  using base_::get_end_pos;
  using base_::contains;
  using base_::get_all_interval;

  using base_::get_batch_size;
  using base_::get_batch_interval;
  using base_::get_batches;
  using base_::get_batch_by_index;

  void swap(input_window& x) {
    base_::swap(static_cast<base_&>(x));
  }
  friend void swap(input_window& x, input_window& y) {
    x.swap(y);
  }

  MSGPACK_DEFINE(batches_, start_pos_, batch_interval_);
};

}  // namespace burst
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_BURST_INPUT_WINDOW_HPP_
