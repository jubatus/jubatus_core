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

#ifndef JUBATUS_CORE_UNLEARNER_UNLEARNER_FACTORY_HPP_
#define JUBATUS_CORE_UNLEARNER_UNLEARNER_FACTORY_HPP_

#include <string>
#include "../common/jsonconfig.hpp"
#include "jubatus/util/lang/shared_ptr.h"
#include "jubatus/util/data/optional.h"
#include "unlearner_config.hpp"

namespace jubatus {
namespace core {
namespace unlearner {

class unlearner_base;

util::lang::shared_ptr<unlearner_base> create_unlearner(
    util::lang::shared_ptr<unlearner_config_base> config);

}  // namespace unlearner
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_UNLEARNER_UNLEARNER_FACTORY_HPP_
