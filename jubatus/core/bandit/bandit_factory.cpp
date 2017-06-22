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

#include "bandit_factory.hpp"

#include <string>
#include "../common/jsonconfig.hpp"
#include "epsilon_greedy.hpp"
#include "ucb1.hpp"
#include "ts.hpp"
#include "softmax.hpp"
#include "exp3.hpp"

using jubatus::util::lang::shared_ptr;
using jubatus::core::common::jsonconfig::config_cast_check;
namespace json = jubatus::util::text::json;

namespace jubatus {
namespace core {
namespace bandit {

shared_ptr<bandit_base> bandit_factory::create(
    const std::string& name,
    const common::jsonconfig::config& param) {
  if (name == "epsilon_greedy") {
    if (param.type() == json::json::Null) {
      throw JUBATUS_EXCEPTION(
          common::config_exception() << common::exception::error_message(
              "parameter block is not specified in config"));
    }
    epsilon_greedy::config conf =
      config_cast_check<epsilon_greedy::config>(param);
    return shared_ptr<bandit_base>(
        new epsilon_greedy(conf));
  } else if (name == "ucb1") {
    if (param.type() == json::json::Null) {
      throw JUBATUS_EXCEPTION(
          common::config_exception() << common::exception::error_message(
              "parameter block is not specified in config"));
    }
    ucb1::config conf =
      config_cast_check<ucb1::config>(param);
    return shared_ptr<bandit_base>(new ucb1(conf));
  } else if (name == "ts") {
    if (param.type() == json::json::Null) {
      throw JUBATUS_EXCEPTION(
          common::config_exception() << common::exception::error_message(
              "parameter block is not specified in config"));
    }
    ts::config conf =
      config_cast_check<ts::config>(param);
    return shared_ptr<bandit_base>(new ts(conf));
  } else if (name == "softmax") {
    if (param.type() == json::json::Null) {
      throw JUBATUS_EXCEPTION(
          common::config_exception() << common::exception::error_message(
              "parameter block is not specified in config"));
    }
    softmax::config conf = config_cast_check<softmax::config>(param);
    return shared_ptr<bandit_base>(
        new softmax(conf));
  } else if (name == "exp3") {
    if (param.type() == json::json::Null) {
      throw JUBATUS_EXCEPTION(
          common::config_exception() << common::exception::error_message(
              "parameter block is not specified in config"));
    }
    exp3::config conf = config_cast_check<exp3::config>(param);
    return shared_ptr<bandit_base>(
        new exp3(conf));
  } else {
    throw JUBATUS_EXCEPTION(
        common::unsupported_method("bandit(" + name + ")"));
  }
}

}  // namespace bandit
}  // namespace core
}  // namespace jubatus
