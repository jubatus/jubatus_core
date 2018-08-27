// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2012 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#ifndef JUBATUS_CORE_REGRESSION_REGRESSION_BASE_HPP_
#define JUBATUS_CORE_REGRESSION_REGRESSION_BASE_HPP_

#include <map>
#include <string>
#include <vector>
#include "jubatus/util/lang/shared_ptr.h"
#include "../common/type.hpp"
#include "../framework/linear_function_mixer.hpp"

namespace jubatus {
namespace core {

namespace storage {
class storage_base;
}  // namespace storage

namespace regression {

class regression_base {
 public:
  regression_base() {
  }
  explicit regression_base(storage_ptr storage) {
  }

  virtual ~regression_base() {
  }

  virtual void train(const common::sfv_t& fv, const double value) = 0;
  virtual double estimate(const common::sfv_t& fv) const = 0;

  virtual void clear() = 0;

  // TODO(beam2d): Think the objective of this function and where it should be
  // defined. Algorithms have |get_status| tentatively to extract status from
  // storages.
  virtual void get_status(std::map<std::string, std::string>& status) const = 0;
  virtual void pack(framework::packer& pk) const = 0;
  virtual void unpack(msgpack::object o) = 0;

  virtual std::vector<framework::mixable*> get_mixables() = 0;
  // Currently only nearest neighbor regression uses unlearner.
  virtual void set_unlearner(
      jubatus::util::lang::shared_ptr<unlearner::unlearner_base>
      label_unlearner) {
  }
};

}  // namespace regression

}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_REGRESSION_REGRESSION_BASE_HPP_
