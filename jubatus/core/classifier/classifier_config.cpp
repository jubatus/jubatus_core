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

#include "classifier_config.hpp"

#include "jubatus/util/data/serialization.h"
#include "jubatus/util/data/optional.h"
#include "jubatus/util/lang/shared_ptr.h"
#include "../common/jsonconfig.hpp"
#include "../unlearner/unlearner_config.hpp"
#include "../storage/column_table.hpp"
#include "../nearest_neighbor/nearest_neighbor_base.hpp"

using jubatus::util::lang::shared_ptr;
using jubatus::core::common::jsonconfig::config_cast_check;

namespace jubatus {
namespace core {
namespace classifier {
classifier_config::classifier_config(const std::string& method,
                                     const common::jsonconfig::config& param) {
  if (method == "perceptron" ||
      method == "PA" || method == "passive_aggressive") {
    if (param.type() != jubatus::util::text::json::json::Null) {
      throw JUBATUS_EXCEPTION(
          common::config_exception() << common::exception::error_message(
              "parameter block is specified in config"));
    }
    conf_.reset(new detail::unlearning_classifier_config(method, param));
  } else if (method == "PA1" || method == "passive_aggressive_1" ||
      method == "PA2" || method == "passive_aggressive_2" ||
      method == "CW" || method == "confidence_weighted" ||
      method == "AROW" || method == "arow" ||
      method == "NHERD" || method == "normal_herd") {
    if (param.type() == jubatus::util::text::json::json::Null) {
      throw JUBATUS_EXCEPTION(
          common::config_exception() << common::exception::error_message(
              "parameter block is not specified in config"));
    }
    conf_.reset(new detail::unlearning_classifier_config(method, param));
  } else if (method == "NN" || method == "nearest_neighbor") {
    if (param.type() == jubatus::util::text::json::json::Null) {
      throw JUBATUS_EXCEPTION(
          common::config_exception() << common::exception::error_message(
              "parameter block is not specified in config"));
    }
    conf_.reset(new detail::nearest_neighbor_classifier_config(method, param));
  } else {
    throw JUBATUS_EXCEPTION(
        common::unsupported_method("classifier(" + method + ")"));
  }
}

}  // namespace classifier
}  // namespace core
}  // namespace jubatus
