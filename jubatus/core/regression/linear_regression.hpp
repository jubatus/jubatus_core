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

#ifndef JUBATUS_CORE_REGRESSION_LINEAR_REGRESSION_HPP_
#define JUBATUS_CORE_REGRESSION_LINEAR_REGRESSION_HPP_

#include <map>
#include <string>
#include <vector>
#include "jubatus/util/lang/shared_ptr.h"
#include "../common/type.hpp"
#include "../framework/linear_function_mixer.hpp"
#include "regression_base.hpp"

namespace jubatus {
namespace core {

namespace storage {
class storage_base;
}  // namespace storage

namespace regression {

class linear_regression : public regression_base {
 public:
  explicit linear_regression(storage_ptr storage);

  virtual void train(const common::sfv_t& fv, const float value) = 0;
  float estimate(const common::sfv_t& fv) const;

  void clear();

  // TODO(beam2d): Think the objective of this function and where it should be
  // defined. Algorithms have |get_status| tentatively to extract status from
  // storages.
  void get_status(std::map<std::string, std::string>& status) const;
  std::vector<framework::mixable*> get_mixables();

  void pack(framework::packer& pk) const;
  void unpack(msgpack::object o);

 protected:
  void update(const common::sfv_t& fv, float coeff);
  float calc_variance(const common::sfv_t& sfv) const;
  storage_ptr storage_;
  framework::linear_function_mixer mixable_storage_;
};

}  // namespace regression
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_REGRESSION_LINEAR_REGRESSION_HPP_
