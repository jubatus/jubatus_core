// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2012 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#include "passive_aggressive_2.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace jubatus {
namespace core {
namespace regression {

passive_aggressive_2::passive_aggressive_2(
    const config& config,
    storage_ptr storage)
    : linear_regression(storage),
      config_(config),
      sum_(0.0),
      sq_sum_(0.0),
      count_(0.0) {

  if (!(0.f < config.regularization_weight)) {
    throw JUBATUS_EXCEPTION(
        common::invalid_parameter("0.0 < regularization_weight"));
  }

  if (!(0.f <= config.sensitivity)) {
    throw JUBATUS_EXCEPTION(
        common::invalid_parameter("0.0 <= sensitivity"));
  }
}

passive_aggressive_2::passive_aggressive_2(storage_ptr storage)
    : linear_regression(storage),
      sum_(0.0),
      sq_sum_(0.0),
      count_(0.0) {
}

static double squared_norm(const common::sfv_t& fv) {
  double norm = 0.0;
  for (size_t i = 0; i < fv.size(); ++i) {
    norm += fv[i].second * fv[i].second;
  }
  return norm;
}

void passive_aggressive_2::train(const common::sfv_t& fv, double value) {
  sum_ += value;
  sq_sum_ += value * value;
  count_ += 1;
  double avg = sum_ / count_;
  double std_dev = std::sqrt(sq_sum_ / count_ -  avg * avg);

  double predict = estimate(fv);
  double error = value - predict;
  double sign_error = error > 0.0 ? 1.0 : -1.0;
  double loss = sign_error * error - config_.sensitivity * std_dev;

  if (loss > 0.0) {
    float C = config_.regularization_weight;
    double coeff = sign_error *  loss / (squared_norm(fv) + 1 / 2 * C);
    if (!std::isinf(coeff)) {
      update(fv, coeff);
    }
  }
}

void passive_aggressive_2::clear() {
  linear_regression::clear();
  sum_ = 0.0;
  sq_sum_ = 0.0;
  count_ = 0.0;
}

}  // namespace regression
}  // namespace core
}  // namespace jubatus
