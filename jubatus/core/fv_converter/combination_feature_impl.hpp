// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2015 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#ifndef JUBATUS_CORE_FV_CONVERTER_COMBINATION_FEATURE_IMPL_HPP_
#define JUBATUS_CORE_FV_CONVERTER_COMBINATION_FEATURE_IMPL_HPP_

#include <string>
#include <utility>
#include "../common/type.hpp"
#include "combination_feature.hpp"

namespace jubatus {
namespace core {
namespace fv_converter {

class combination_add_feature : public combination_feature {
 public:
  void add_feature(const std::string& key,
                   double value_left,
                   double value_right,
                   common::sfv_t& ret_fv) const {
    ret_fv.push_back(
        std::make_pair(key, static_cast<float>(value_left + value_right)));
  }

  bool is_commutative() const {
    return true;
  }
};

class combination_mul_feature : public combination_feature {
 public:
  void add_feature(const std::string& key,
                   double value_left,
                   double value_right,
                   common::sfv_t& ret_fv) const {
    ret_fv.push_back(
        std::make_pair(key, static_cast<float>(value_left * value_right)));
  }

  bool is_commutative() const {
    return true;
  }
};

}  // namespace fv_converter
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_FV_CONVERTER_COMBINATION_FEATURE_IMPL_HPP_
