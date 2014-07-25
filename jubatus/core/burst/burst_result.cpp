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

#include "burst_result.hpp"

#include <vector>
#include <utility>

#include "input_window.hpp"
#include "result_window.hpp"
#include "engine.hpp"
#include "window_intersection.hpp"

namespace jubatus {
namespace core {
namespace burst {

burst_result::burst_result() {
}

burst_result::burst_result(
    const input_window& input,
    double scaling_param,
    double gamma,
    double costcut_threshold,
    const burst_result& prev_result,
    int max_reuse_batches) {
  const std::vector<batch_input>& input_batches = input.get_batches();
  const size_t n = input.get_batch_size();
  const int max_reuse = (std::min)(max_reuse_batches, static_cast<int>(n));

  // make vectors for engine
  std::vector<uint32_t> d_vec, r_vec;
  std::vector<double> burst_weights;
  d_vec.reserve(n);
  r_vec.reserve(n);
  burst_weights.reserve(n);
  for (size_t i = 0; i < n; ++i) {
    d_vec.push_back(input_batches[i].d);
    r_vec.push_back(input_batches[i].r);
    burst_weights.push_back(-1);  // uncalculated
  }

  // reuse batch weights
  if (prev_result.p_) {
    const result_window& prev = *prev_result.p_;
    if (prev.get_start_pos() <= input.get_start_pos()) {
      const std::pair<int, int> intersection = get_intersection(prev, input);
      const std::vector<batch_result>& prev_results = prev.get_batches();
      for (int i = 0, j = intersection.first;
           i < max_reuse && j < intersection.second;
           ++i, ++j) {
        burst_weights[i] = prev_results[j].burst_weight;
      }
    }
  }

  // doit
  burst::burst_detect(d_vec, r_vec, burst_weights,
                      scaling_param, gamma, costcut_threshold);

  // store result
  p_.reset(new result_window(input, burst_weights));
}

burst_result::burst_result(const result_window& src)
    : p_(new result_window(src)) {
}

bool burst_result::is_valid() const {
  return p_.get() != NULL;
}

const double burst_result::invalid_pos = -1;

double burst_result::get_start_pos() const {
  return p_ ? p_->get_start_pos() : invalid_pos;
}
double burst_result::get_end_pos() const {
  return p_ ? p_->get_end_pos() : invalid_pos;
}

bool burst_result::contains(double pos) const {
  return p_ && p_->contains(pos);
}

double burst_result::get_all_length() const {
  return p_ ? p_->get_all_length() : 0;
}

int burst_result::get_batch_size() const {
  return p_ ? p_->get_batch_size() : 0;
}
double burst_result::get_batch_length() const {
  return p_ ? p_->get_batch_length() : 0;
}

const std::vector<batch_result> empty_batch_results;

const std::vector<batch_result>& burst_result::get_batches() const {
  return p_ ? p_->get_batches() : empty_batch_results;
}

bool burst_result::is_bursted_at(double pos) const {
  int i = p_ ? p_->get_index(pos) : -1;
  if (i < 0) {
    throw std::out_of_range("burst_result: pos is out of range");
  }
  return p_->get_batches()[i].is_bursted();
}

bool burst_result::is_bursted_at_latest_batch() const {
  if (!p_) {
    return false;
  }
  const std::vector<batch_result>& batches = p_->get_batches();
  return !batches.empty() && batches.back().is_bursted();
}

}  // namespace burst
}  // namespace core
}  // namespace jubatus
