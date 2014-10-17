// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2014 Preferred Infrastructure and Nippon Telegraph and Telephone Corporation.
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

#include "epsilon_greedy.hpp"
#include "ucb1.hpp"

using jubatus::util::lang::shared_ptr;
using jubatus::core::common::jsonconfig::config_cast_check;
namespace json = jubatus::util::text::json;

namespace jubatus {
namespace core {
namespace bandit {

struct epsilon_greedy_config {
  double epsilon;

  template<class Ar>
  void serialize(Ar& ar) {
    ar & JUBA_MEMBER(epsilon);
  }
};

shared_ptr<bandit_base> bandit_factory::create(
    const std::string& name,
    const common::jsonconfig::config& param,
    const shared_ptr<storage>& s) {
  if (name == "epsilon_greedy") {
    if (param.type() == json::json::Null) {
      throw JUBATUS_EXCEPTION(
          common::config_exception() << common::exception::error_message(
              "parameter block is not specified in config"));
    }
    epsilon_greedy_config conf =
        config_cast_check<epsilon_greedy_config>(param);
    return shared_ptr<bandit_base>(new epsilon_greedy(s, conf.epsilon));
  } else if (name == "ucb1") {
    return shared_ptr<bandit_base>(new ucb1(s));
  } else {
    throw JUBATUS_EXCEPTION(
        common::unsupported_method("bandit(" + name + ")"));
  }
}

}  // namespace bandit
}  // namespace core
}  // namespace jubatus
