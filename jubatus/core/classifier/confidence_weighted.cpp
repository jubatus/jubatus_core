// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2011 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#include "jubatus/util/concurrent/lock.h"
#include "classifier_util.hpp"
#include "../common/exception.hpp"

using std::string;

namespace jubatus {
namespace core {
namespace classifier {

confidence_weighted::confidence_weighted(storage_ptr storage)
    : linear_classifier(storage) {
}

confidence_weighted::confidence_weighted(
    const classifier_config& config,
    storage_ptr storage)
    : linear_classifier(storage),
      config_(config) {

  if (!(0.f < config.regularization_weight)) {
    throw JUBATUS_EXCEPTION(
        common::invalid_parameter("0.0 < regularization_weight"));
  }
}

void confidence_weighted::train(const common::sfv_t& sfv, const string& label) {
  check_touchable(label);

  labels_.get_model()->increment(label);

  const float C = config_.regularization_weight;
  string incorrect_label;
  double variance = 0.0;
  double margin = -calc_margin_and_variance(sfv, label, incorrect_label,
                                            variance);
  double b = 1.0 + 2 * C * margin;
  double gamma = -b + std::sqrt(b * b - 8 * C * (margin - C * variance));

  if (gamma <= 0.0) {
    storage_->register_label(label);
    return;
  }
  gamma /= 4 * C * variance;
  update(sfv, gamma, label, incorrect_label);
}

void confidence_weighted::update(
    const common::sfv_t& sfv,
    double step_width,
    const string& pos_label,
    const string& neg_label) {
  util::concurrent::scoped_wlock lk(storage_->get_lock());
  for (common::sfv_t::const_iterator it = sfv.begin(); it != sfv.end(); ++it) {
    const string& feature = it->first;
    double val = it->second;
    storage::feature_val2_t val2;
    storage_->get2_nolock(feature, val2);

    storage::val2_t pos_val(0.0, 1.0);
    storage::val2_t neg_val(0.0, 1.0);
    ClassifierUtil::get_two(val2, pos_label, neg_label, pos_val, neg_val);

    const float C = config_.regularization_weight;
    double covar_pos_step = 2.0 * step_width * val * val * C;
    double covar_neg_step = 2.0 * step_width * val * val * C;

    storage_->set2_nolock(
        feature,
        pos_label,
        storage::val2_t(pos_val.v1 + step_width * pos_val.v2 * val,
                        1.0 / (1.0 / pos_val.v2 + covar_pos_step)));
    if (neg_label != "") {
      storage_->set2_nolock(
          feature,
          neg_label,
          storage::val2_t(neg_val.v1 - step_width * neg_val.v2 * val,
                          1.0 / (1.0 / neg_val.v2 + covar_neg_step)));
    }
  }
  touch(pos_label);
}

string confidence_weighted::name() const {
  return string("confidence_weighted");
}

}  // namespace classifier
}  // namespace core
}  // namespase jubatus
