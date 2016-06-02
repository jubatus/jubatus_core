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

#ifndef JUBATUS_CORE_FV_CONVERTER_KEY_NAME_UTILS_HPP_
#define JUBATUS_CORE_FV_CONVERTER_KEY_NAME_UTILS_HPP_

#include <string>

/*
 * This header provides utility functions to manipulate feature vector keys.
 *
 * Feature vector keys are formatted as:
 *
 * String features:
 *    "<KEY_NAME>$<VALUE>@<FEATURE_TYPE>#<SAMPLE_WEIGHT>/<GLOBAL_WEIGHT>"
 * Numeric features:
 *    "<KEY_NAME>@<FEATURE_TYPE>"
 * Binary features:
 *    "<KEY_NAME>"
 */

namespace jubatus {
namespace core {
namespace fv_converter {

/**
 * Extract the group key name from a string feature vector key.
 *
 * The "group key name" is a string feature vector key name without value.
 *
 * - string: "key$value@rule#sw/gw" -> "key@rule#sw/gw"
 * - number: "key@rule" -> undefined ("" in most cases)
 * - binary: "key" -> undefined ("" in most cases)
 *
 * Note: For performance reasons, this function assumes that the feature
 * extraction rule name does not contains "@".  See also `revert.cpp`.
 */
inline std::string get_group_key_from_key(
    const std::string& key) {
  size_t at = key.rfind('@');
  if (at == std::string::npos) {
    return "";  // binary feature
  }
  size_t dollar = key.find('$');
  if (dollar == std::string::npos) {
    return "";  // numeric feature
  }
  return key.substr(0, dollar) + key.substr(at);
};

/**
 * Extract the global_weight rule from feature vector key.  For non-string
 * feature vector keys returns an empty string.
 *
 * - string: "key$value@rule#sw/gw" -> "gw"
 * - number: "key@rule" -> undefined ("" in most cases)
 * - binary: "key" -> undefined ("" in most cases)
 *
 * Note: For number/binary feature keys, the resulting value is undefined.
 * If the caller uses this method for these kind of keys, it is advised to
 * ignore unknown global weight types, instead of reporting an error to the user.
 */
inline std::string get_global_weight_type_from_key(
    const std::string& key) {
  size_t p = key.find_last_of('/');
  if (p != std::string::npos) {
    return key.substr(p + 1);
  }
  return "";
};

}  // namespace fv_converter
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_FV_CONVERTER_KEY_NAME_UTILS_HPP_
