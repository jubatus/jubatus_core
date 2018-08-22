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

#include "perceptron.hpp"

#include <iostream>
namespace jubatus {
namespace core {
namespace regression {

perceptron::perceptron(
    const config& config,
    storage_ptr storage)
  : linear_regression(storage),
    config_(config) {
  if (!(0.f < config.learning_rate)) {
    throw JUBATUS_EXCEPTION(
        common::invalid_parameter("0.0 < learning_rate"));
  }
}


perceptron::perceptron(
    storage_ptr storage)
  : linear_regression(storage),
    config_(config()) {
}

void perceptron::train(const common::sfv_t& fv, double value) {
  double predict = estimate(fv);
  std::cerr << "predict:" << predict << std::endl;
  double error = value - predict;
  update(fv, error * config_.learning_rate);
}

void perceptron::clear() {
  linear_regression::clear();
}

}  // namespace regression
}  // namespace core
}  // namespace jubatus
