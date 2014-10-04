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

#include <map>
#include <string>
#include "jubatus/util/lang/cast.h"
#include "exception.hpp"
#include "num_filter_factory.hpp"
#include "num_filter_impl.hpp"
#include "util.hpp"

using jubatus::util::lang::shared_ptr;

namespace jubatus {
namespace core {
namespace fv_converter {

namespace {
shared_ptr<add_filter> create_add_filter(
    const std::map<std::string, std::string>& params) {
  const std::string& value = get_or_die(params, "value");
  double float_val = jubatus::util::lang::lexical_cast<double>(value);
  return shared_ptr<add_filter>(new add_filter(float_val));
}

shared_ptr<linear_normalization_filter> create_linear_normalization_filter(
    const std::map<std::string, std::string>& params) {
  const std::string& min = get_or_die(params, "min");
  const std::string& max = get_or_die(params, "max");
  const std::string truncate = get_with_default(params, "truncate", "True");
  const double float_min = jubatus::util::lang::lexical_cast<double>(min);
  const double float_max = jubatus::util::lang::lexical_cast<double>(max);
  const bool truncate_flag = truncate == "True";
  return shared_ptr<linear_normalization_filter>(
      new linear_normalization_filter(float_min, float_max, truncate_flag));
}

shared_ptr<gaussian_normalization_filter> create_gaussian_normalization_filter(
    const std::map<std::string, std::string>& params) {
  const std::string& avg = get_or_die(params, "average");
  const std::string& var = get_or_die(params, "variance");
  const double float_avg = jubatus::util::lang::lexical_cast<double>(avg);
  const double float_var = jubatus::util::lang::lexical_cast<double>(var);
  return shared_ptr<gaussian_normalization_filter>(
      new gaussian_normalization_filter(float_avg, float_var));
}

shared_ptr<sigmoid_normalization_filter> create_sigmoid_normalization_filter(
    const std::map<std::string, std::string>& params) {
  const std::string gain = get_with_default(params, "gain", "1");
  const std::string bias = get_with_default(params, "bias", "0");
  const double float_gain = jubatus::util::lang::lexical_cast<double>(gain);
  const double float_bias = jubatus::util::lang::lexical_cast<double>(bias);
  return shared_ptr<sigmoid_normalization_filter>(
      new sigmoid_normalization_filter(float_gain, float_bias));
}
}  // namespace

shared_ptr<num_filter> num_filter_factory::create(
    const std::string& name,
    const param_t& params) const {
  num_filter* p;
  if (name == "add") {
    return create_add_filter(params);
  } else if (name == "linear_normalization") {
    return create_linear_normalization_filter(params);
  } else if (name == "gaussian_normalization") {
    return create_gaussian_normalization_filter(params);
  } else if (name == "sigmoid_normalization") {
    return create_sigmoid_normalization_filter(params);
  } else if (ext_ && (p = ext_(name, params))) {
    return shared_ptr<num_filter>(p);
  } else {
    throw JUBATUS_EXCEPTION(
        converter_exception("unknonw num filter name: " + name));
  }
}

}  // namespace fv_converter
}  // namespace core
}  // namespace jubatus
