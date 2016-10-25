// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2016 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#include "normal_herd.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace jubatus {
namespace core {
namespace regression {

normal_herd::normal_herd(
    const config& config,
    storage_ptr storage)
    : linear_regression(storage),
      config_(config) {

  if (!(0.f < config.regularization_weight)) {
    throw JUBATUS_EXCEPTION(
        common::invalid_parameter("0.0 < regularization_weight"));
  }

  if (!(0.f <= config.sensitivity)) {
    throw JUBATUS_EXCEPTION(
        common::invalid_parameter("0.0 <= sensitivity"));
  }
}

normal_herd::normal_herd(storage_ptr storage)
  : linear_regression(storage) {
}

void normal_herd::train(const common::sfv_t& fv, float value) {
  float predict = estimate(fv);
  float error = value - predict;
  float sign_error = error > 0.f ? 1.0f : -1.0f;
  float loss = sign_error * error - config_.sensitivity;
  if (loss > 0.f) {
    float variance = calc_variance(fv);
    update(fv, loss, variance, sign_error);
  }
}

void normal_herd::update(
    const common::sfv_t& sfv,
    float loss,
    float variance,
    float sign_error) {
  util::concurrent::scoped_wlock lk(storage_->get_lock());
  for (common::sfv_t::const_iterator it = sfv.begin(); it != sfv.end(); ++it) {
    const std::string& feature = it->first;
    float val = it->second;
    storage::feature_val2_t val2;
    storage_->get2_nolock(feature, val2);
    storage::val2_t current_val(0.f, 1.f);
    if (val2.size() > 0) {
      current_val = val2[0].second;
    }
    const float C = config_.regularization_weight;
    float val_covariance = val * current_val.v2;
    storage_->set2_nolock(
        feature,
        "+",
        storage::val2_t(current_val.v1 +
          sign_error * loss * val_covariance
          / (variance + 1.f / C),
        1.f
          / ((1.f / current_val.v2) + (2 * C + C * C * variance)
          * val * val)));
  }
}


void normal_herd::clear() {
  linear_regression::clear();
}

}  // namespace regression
}  // namespace core
}  // namespace jubatus
