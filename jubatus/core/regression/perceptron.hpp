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

#ifndef JUBATUS_CORE_REGRESSION_PERCEPTRON_HPP_
#define JUBATUS_CORE_REGRESSION_PERCEPTRON_HPP_

#include <string>
#include "jubatus/util/data/serialization.h"
#include "linear_regression.hpp"

namespace jubatus {
namespace core {
namespace regression {

class perceptron : public linear_regression {
 public:
  struct config {
    config()
      : learning_rate(0.1f) {
    }
    float learning_rate;

    template<typename Ar>
    void serialize(Ar& ar) {
      ar & JUBA_NAMED_MEMBER("learning_rate", learning_rate);
    }
  };

  perceptron(
      const config& config,
      storage_ptr storage);
  explicit perceptron(storage_ptr storage);
  void train(const common::sfv_t& sfv, const float value);
  void clear();

 private:
  config config_;
};

}  // namespace regression
}  // namespace core
}  // namespace jubatus
#endif  // JUBATUS_CORE_REGRESSION_PERCEPTRON_HPP_
