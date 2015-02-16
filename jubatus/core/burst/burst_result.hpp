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

#ifndef JUBATUS_CORE_BURST_BURST_RESULT_HPP_
#define JUBATUS_CORE_BURST_BURST_RESULT_HPP_

#include <stdint.h>
#include <algorithm>  // for std::swap
#include <vector>
#include <msgpack.hpp>
#include "jubatus/util/lang/shared_ptr.h"

#include "window_fwd.hpp"
#include "../framework/mixable_helper.hpp"

namespace jubatus {
namespace core {
namespace burst {

class burst_result {
 public:
  burst_result();
  burst_result(const input_window& input,
               double scaling_param,
               double gamma,
               double costcut_threshold,
               const burst_result& prev_result,
               int max_reuse_batches);
  // for testing
  explicit burst_result(const result_window& src);

  // copy ctor is not user-defined (uses shared_ptr's one, CoW)
  // window(const window& x);

  void swap(burst_result& x) {
    p_.swap(x.p_);
  }
  friend void swap(burst_result& x, burst_result& y) {
    x.swap(y);
  }

  bool is_valid() const;

  static const double invalid_pos;  // = -1
  double get_start_pos() const;
  double get_end_pos() const;
  bool contains(double pos) const;

  int get_batch_size() const;
  double get_batch_interval() const;
  double get_all_interval() const;

  // comparing start_pos
  // floating number error is considered
  bool has_start_pos_older_than(double start_pos) const;
  bool has_start_pos_newer_than(double start_pos) const;
  bool has_same_start_pos_to(double start_pos) const;

  // compares batch_interval; floating number error is allowed
  bool has_same_batch_interval(const burst_result& x) const;

  const std::vector<batch_result>& get_batches() const;

  const batch_result& get_batch_at(double pos) const;
  bool is_bursted_at(double pos) const;
  bool is_bursted_at_latest_batch() const;

  bool mix(const burst_result& w);

  void msgpack_pack(framework::packer& packer) const;
  void msgpack_unpack(msgpack::object o);

 private:
  jubatus::util::lang::shared_ptr<const result_window> p_;
};

}  // namespace burst
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_BURST_BURST_RESULT_HPP_
