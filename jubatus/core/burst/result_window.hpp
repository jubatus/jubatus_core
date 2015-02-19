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

#ifndef JUBATUS_CORE_BURST_RESULT_WINDOW_HPP_
#define JUBATUS_CORE_BURST_RESULT_WINDOW_HPP_

#include <vector>

#include "input_window.hpp"

namespace jubatus {
namespace core {
namespace burst {

class result_window : private basic_window<batch_result> {
  typedef basic_window<batch_result> base_;

 public:
  explicit result_window(double start_pos = 0, double batch_interval = 1)
      : base_(start_pos, batch_interval, 0) {
  }

  result_window(const input_window& input,
                const std::vector<double>& burst_weights)
      : base_(input.get_start_pos(),
              input.get_batch_interval(),
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
  using base_::get_all_interval;

  using base_::get_batches;
  using base_::get_batch_size;
  using base_::get_batch_interval;

  MSGPACK_DEFINE(batches_, start_pos_, batch_interval_);
};

}  // namespace burst
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_BURST_RESULT_WINDOW_HPP_
