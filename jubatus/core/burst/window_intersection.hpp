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

#ifndef JUBATUS_CORE_BURST_WINDOW_INTERSECTION_HPP_
#define JUBATUS_CORE_BURST_WINDOW_INTERSECTION_HPP_

#include <stdint.h>
#include <algorithm>
#include <utility>
#include <vector>
#include <limits>
#include <msgpack.hpp>
#include "jubatus/util/lang/shared_ptr.h"

#include "input_window.hpp"
#include "../framework/mixable_helper.hpp"

namespace jubatus {
namespace core {
namespace burst {

inline bool window_position_near(
    double pos0, double pos1, double batch_interval) {
  return std::abs(pos1 - pos0) / batch_interval < 0.01;
}

class intersection_helper {
 public:
  explicit intersection_helper(double start_pos = 0,
                               double batch_interval = 1,
                               int batch_size = 0)
      : start_pos_(start_pos),
        batch_interval_(batch_interval),
        batch_size_(batch_size) {
  }

  template<class Window>
  explicit intersection_helper(const Window& w)
      : start_pos_(w.get_start_pos()),
        batch_interval_(w.get_batch_interval()),
        batch_size_(w.get_batch_size()) {
  }

  template<class Window>
  std::pair<int, int> get_intersection(const Window& w) const {
    int begin = get_index_for_boundary(w.get_start_pos());
    int end = get_index_for_boundary(w.get_end_pos());

    if (begin == no_index() ||
        end == no_index() ||
        !has_batch_interval_equals_to(w.get_batch_interval())) {
      // nonsense data is given (should throw an exception?)
      return std::make_pair(no_index(), no_index());
    }

    return std::make_pair(adjust_index(begin),
                          adjust_index(end));
  }

  int get_index_for_boundary(double boundary_pos) const {
    int candidate = get_index_(boundary_pos + batch_interval_/2);
    double candidate_pos = start_pos_ + candidate * batch_interval_;
    if (!position_near(boundary_pos, candidate_pos)) {
      return no_index();
    }
    return candidate;
  }

  // index value when index cannot be determined
  static int no_index() {  // constexpr is needed
    return std::numeric_limits<int>::min();
  }

  bool position_near(double pos0, double pos1) const {
    return window_position_near(pos0, pos1, batch_interval_);
  }

  bool has_batch_interval_equals_to(double interval1) const {
    // comparing end_pos
    size_t n = (std::max)(batch_size_, 1);
    return position_near(n * interval1, n * batch_interval_);
  }

  int adjust_index(int index) const {
    if (index < 0) {
      return 0;
    } else if (index > batch_size_) {
      return batch_size_;
    } else {
      return index;
    }
  }

  int get_index_(double pos) const {
    return static_cast<int>(std::floor((pos - start_pos_) / batch_interval_));
  }

 private:
  double start_pos_;
  double batch_interval_;
  int batch_size_;
};

template<class W1, class W2>
std::pair<int, int> get_intersection(const W1& w1, const W2& w2) {
  return intersection_helper(w1).get_intersection(w2);
}

}  // namespace burst
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_BURST_WINDOW_INTERSECTION_HPP_
