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

#include "confidence_weighted.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace jubatus {
namespace core {
namespace regression {

confidence_weighted::confidence_weighted(
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

confidence_weighted::confidence_weighted(storage_ptr storage)
  : linear_regression(storage) {
}

void confidence_weighted::train(const common::sfv_t& fv, double value) {
  double predict = estimate(fv);
  double error = value - predict;
  double sign_error = error > 0.0 ? 1.0 : -1.0;
  double loss = sign_error * error - config_.sensitivity;
  double variance = calc_variance(fv);
  double absolute_error = std::fabs(error);
  if (loss > 0.0) {
    float C = config_.regularization_weight;
    double b = 1.0 - 2 * C * absolute_error;
    double gamma = -b + std::sqrt(b * b +
                                  8 * C * (absolute_error + C * variance));
    gamma /= 4 * C * variance;
    update(fv, gamma, sign_error);
  }
}

void confidence_weighted::update(
    const common::sfv_t& sfv,
    double step_width,
    double sign_error) {
  util::concurrent::scoped_wlock lk(storage_->get_lock());
  for (common::sfv_t::const_iterator it = sfv.begin(); it != sfv.end(); ++it) {
    const std::string& feature = it->first;
    double val = it->second;
    storage::feature_val2_t val2;
    storage_->get2_nolock(feature, val2);
    storage::val2_t current_val(0.0, 1.0);
    if (val2.size() > 0) {
      current_val = val2[0].second;
    }
    const float C = config_.regularization_weight;
    double covar_step = 2 * step_width * val * val * C;
    storage_->set2_nolock(
        feature,
        "+",
        storage::val2_t(current_val.v1
              + sign_error * step_width * current_val.v2 * val,
            1.0 / (1.0 / current_val.v2 + covar_step)));
  }
}


void confidence_weighted::clear() {
  linear_regression::clear();
}

}  // namespace regression
}  // namespace core
}  // namespace jubatus
