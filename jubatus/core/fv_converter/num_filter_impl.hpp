// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2011 Preferred Infrastructure and Nippon Telegraph and Telephone Corporation.
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

#ifndef JUBATUS_CORE_FV_CONVERTER_NUM_FILTER_IMPL_HPP_
#define JUBATUS_CORE_FV_CONVERTER_NUM_FILTER_IMPL_HPP_

#include "num_filter.hpp"

namespace jubatus {
namespace core {
namespace fv_converter {

class add_filter : public num_filter {
 public:
  explicit add_filter(double value)
      : value_(value) {
  }

  double filter(double value) const {
    return value + value_;
  }

 private:
  double value_;
};

class linear_normalization_filter : public num_filter {
 public:
  linear_normalization_filter(double max,
                              double min,
                              bool truncate)
    : max_(max), min_(min), truncate_(truncate) {
  }

  double filter(double value) const {
    if (truncate_) {
      if (max_ < value) {
        return 1.0;
      } else if (value < min_) {
        return 0.0;
      }
    }
    return (value - min_) / (max_ - min_);
  }

 private:
  double max_;
  double min_;
  bool truncate_;
};

}  // namespace fv_converter
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_FV_CONVERTER_NUM_FILTER_IMPL_HPP_
