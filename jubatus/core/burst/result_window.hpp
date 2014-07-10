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

#ifndef JUBATUS_CORE_BURST_RESULT_WINDOW_HPP_
#define JUBATUS_CORE_BURST_RESULT_WINDOW_HPP_

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

class result_window : private basic_window<batch_result> {
  typedef basic_window<batch_result> base_;

 public:
  explicit result_window(double start_pos = 0, double batch_length = 1)
      : base_(start_pos, batch_length, 0) {
  }

  result_window(const input_window& input,
                const std::vector<double>& burst_weights)
      : base_(input.get_start_pos(),
              input.get_batch_length(),
              input.get_batch_size()) {
    size_t n = input.get_batch_size();

    if (burst_weights.size() != n) {
      throw JUBATUS_EXCEPTION(common::invalid_parameter(
          "result_window: size of burst_weights is not matching"));
    }

    const std::vector<batch_input>& inputs = input.get_batches();
    for (size_t i = 0; i < n; ++i) {
      batches_[i] = batch_result(inputs[i], burst_weights[i]);
    }
  }

  using base_::get_index;

  using base_::get_start_pos;
  using base_::get_end_pos;
  using base_::contains;
  using base_::get_all_length;

  using base_::get_batches;
  using base_::get_batch_size;
  using base_::get_batch_length;

  // get indices intersects with argumemt window
  // precond: w.get_start_pos() <= w.get_end_pos()
  template<class Window>
  std::pair<int, int>
      get_intersection(const Window& w) const {
    int begin = get_index_for_intersection(w.get_start_pos());
    int end = get_index_for_intersection(w.get_end_pos());

    if (begin == no_index() ||
        end == no_index() ||
        !has_batch_length_equals_to(w.get_batch_length())) {
      // nonsense data is given (should throw an exception?)
      return std::make_pair(no_index(), no_index());
    }

    return std::make_pair(adjust_index_for_intersection(begin),
                          adjust_index_for_intersection(end));
  }

  int get_index_for_intersection(double start_pos) const {
    double batch_length = get_batch_length();
    int candidate = base_::get_index_(start_pos + batch_length/2);
    double candidate_pos = get_start_pos() + candidate * batch_length;
    if (!position_near(start_pos, candidate_pos)) {
      return no_index();
    }
    return candidate;
  }

  // index value when index cannot be determined
  static int no_index() {  // constexpr is needed
    return std::numeric_limits<int>::min();
  }

  bool position_near(double pos1, double pos0) const {
    return std::abs(pos1 - pos0) / get_batch_length() < 0.01;
  }

  bool has_batch_length_equals_to(double length1) const {
    size_t n = (std::max)(get_batch_size(), 1);
    double length0 = get_batch_length();
    return position_near(n * length1, n * length0);  // comparing end_pos
  }

  int adjust_index_for_intersection(int index) const {
    if (index < 0) {
      return 0;
    } else if (index > get_batch_size()) {
      return get_batch_size();
    } else {
      return index;
    }
  }
};

}  // namespace burst
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_BURST_RESULT_WINDOW_HPP_
