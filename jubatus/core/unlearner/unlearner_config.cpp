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

#include "unlearner_config.hpp"
#include <string>
#include "../common/jsonconfig.hpp"
#include "jubatus/util/lang/shared_ptr.h"
#include "jubatus/util/data/optional.h"
#include "lru_unlearner.hpp"
#include "random_unlearner.hpp"

namespace jubatus {
namespace core {
namespace unlearner {

util::lang::shared_ptr<unlearner_config_base>
create_unlearner_config(const std::string name,
                        const common::jsonconfig::config& config) {
  if (name == "lru") {
    return util::lang::shared_ptr<unlearner_base>(
        new lru_unlearner(common::jsonconfig::config_cast_check<
                          lru_unlearner::lru_unlearner_config>(config)));
  } else if (name == "random") {
    return util::lang::shared_ptr<unlearner_base>(
        new random_unlearner(
            common::jsonconfig::config_cast_check<
                random_unlearner::random_unlearner_config>(config)));
  } else {
    throw JUBATUS_EXCEPTION(common::unsupported_method(
                                "unlearner(" + name + ')'));
  }
}

}  // namespace unlearner
}  // namespace core
}  // namespace jubatus
