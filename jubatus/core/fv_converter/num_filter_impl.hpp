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

#include <cmath>
#include "num_filter.hpp"
#include "../common/exception.hpp"

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
  linear_normalization_filter(double min,
                              double max,
                              bool truncate)
    : min_(min), max_(max), truncate_(truncate) {
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
  double min_;
  double max_;
  bool truncate_;
};

class gaussian_normalization_filter : public num_filter {
 public:
  gaussian_normalization_filter(double average,
                                double variance)
    : average_(average), variance_(variance) {
    if (variance_ < 0) {
      throw JUBATUS_EXCEPTION(
          common::invalid_parameter("Variance must be non-negative"));
    }
  }

  double filter(double value) const {
    return (value - average_) / variance_;
  }
 private:
  double average_;
  double variance_;
};

class sigmoid_normalization_filter : public num_filter {
 public:
  sigmoid_normalization_filter(double gain,
                               double bias)
    : bias_(bias), gain_(gain) {
    if (gain_ == 0.0) {
      throw JUBATUS_EXCEPTION(
          common::invalid_parameter("gain must not be zero"));
    }
  }

  double filter(double value) const {
    return 1.0 / (1 + std::exp(-gain_ * (value - bias_)));
  }

 private:
  double bias_;
  double gain_;
};

}  // namespace fv_converter
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_FV_CONVERTER_NUM_FILTER_IMPL_HPP_
