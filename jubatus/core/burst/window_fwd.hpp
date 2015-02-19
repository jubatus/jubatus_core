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

#ifndef JUBATUS_CORE_BURST_WINDOW_FWD_HPP_
#define JUBATUS_CORE_BURST_WINDOW_FWD_HPP_

#include <stdint.h>
#include <msgpack.hpp>

namespace jubatus {
namespace core {
namespace burst {

struct batch_input {
  int32_t d;  // all data count
  int32_t r;  // relevant data count

  batch_input()
      : d(0), r(0) {
  }

  batch_input(int32_t d_, int32_t r_)
      : d(d_), r(r_) {
  }

  MSGPACK_DEFINE(d, r);
};

class input_window;

struct batch_result {
  int32_t d, r;
  double burst_weight;  // -1 if unanalysed

  batch_result()
      : d(0), r(0), burst_weight(-1) {
  }

  batch_result(int32_t d_, int32_t r_, double w = -1)
      : d(d_), r(r_), burst_weight(w) {
  }

  explicit batch_result(const batch_input& input, double w = -1)
      : d(input.d), r(input.r), burst_weight(w) {
  }

  MSGPACK_DEFINE(d, r, burst_weight);

  bool is_bursted() const {
    return burst_weight > 0;
  }
};

class result_window;

}  // namespace burst
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_BURST_WINDOW_FWD_HPP_
