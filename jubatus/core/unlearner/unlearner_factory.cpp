// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2013 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#include "unlearner_factory.hpp"

#include <string>
#include "jubatus/util/lang/shared_ptr.h"
#include "../common/exception.hpp"
#include "lru_unlearner.hpp"
#include "random_unlearner.hpp"

using jubatus::util::lang::shared_ptr;

namespace jubatus {
namespace core {
namespace unlearner {

shared_ptr<unlearner_base> create_unlearner(
    const shared_ptr<unlearner_config_base> conf) {
  if (conf->name == "lru") {
    lru_unlearner::lru_unlearner_config* lconf =
      dynamic_cast<lru_unlearner::lru_unlearner_config*>(conf.get());
    if (lconf) {
      return shared_ptr<unlearner_base>(
        new lru_unlearner(*lconf));
    } else {
      throw JUBATUS_EXCEPTION(common::unsupported_method(
                                  "invaild lru unlearner config"));
    }
  } else if (conf->name == "random") {
    random_unlearner::random_unlearner_config* rconf =
      dynamic_cast<random_unlearner::random_unlearner_config*>(conf.get());
    if (rconf) {
      return shared_ptr<unlearner_base>(
        new random_unlearner(*rconf));
    } else {
      throw JUBATUS_EXCEPTION(common::unsupported_method(
                                  "invaild random unlearner config"));
    }
  } else {
    throw JUBATUS_EXCEPTION(common::unsupported_method(
                                "unlearner(" + conf->name + ')'));
  }
}

}  // namespace unlearner
}  // namespace core
}  // namepsace jubatus
