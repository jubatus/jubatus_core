// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2015 Preferred Infrastructure and Nippon Telegraph and Telephone Corporation.
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

#ifndef JUBATUS_CORE_DRIVER_WORDCOUNT_UTIL_HPP_
#define JUBATUS_CORE_DRIVER_WORDCOUNT_UTIL_HPP_

#include <string>
#include <utility>
#include <vector>
#include "jubatus/util/data/unordered_map.h"
#include "jubatus/util/lang/shared_ptr.h"
#include "../wordcount/wordcount_base.hpp"
#include "../fv_converter/datum.hpp"
#include "../fv_converter/exception.hpp"
#include "../framework/mixable.hpp"
#include "../framework/diffv.hpp"
#include "../fv_converter/mixable_weight_manager.hpp"
#include "../fv_converter/datum_to_fv_converter.hpp"
#include "driver.hpp"

namespace jubatus {
namespace core {
namespace driver {
namespace util {

std::string revert_string_feature(const std::string& feature) {
    // format of string feature is
    // "<KEY_NAME>$<VALUE>@<FEATURE_TYPE>#<SAMPLE_WEIGHT>/<GLOBAL_WEIGHT>"
  std::vector<std::string> ret;
  size_t sharp = feature.rfind('#');
  if (sharp == std::string::npos) {
    throw JUBATUS_EXCEPTION(
        fv_converter::converter_exception(
            "this feature is not string feature"));
  }
  size_t at = feature.rfind('@', sharp);
  if (at == std::string::npos) {
    throw JUBATUS_EXCEPTION(
        fv_converter::converter_exception(
            "this feature is not valid feature"));
  }
  size_t dollar = feature.rfind('$', at);
  if (dollar == std::string::npos) {
    throw JUBATUS_EXCEPTION(
        fv_converter::converter_exception(
            "this feature is not valid feature"));
  }

  return feature.substr(dollar + 1, at - dollar - 1);
}

}  // namespace util
}  // namespace driver
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_DRIVER_WORDCOUNT_HPP_
