// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2011,2012 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#ifndef JUBATUS_CORE_DRIVER_REGRESSION_HPP_
#define JUBATUS_CORE_DRIVER_REGRESSION_HPP_

#include <map>
#include <string>
#include <utility>

#include "jubatus/util/lang/shared_ptr.h"
#include "../framework/linear_function_mixer.hpp"
#include "../fv_converter/mixable_weight_manager.hpp"
#include "driver.hpp"

namespace jubatus {
namespace core {
namespace fv_converter {
class datum;
class datum_to_fv_converter;
}  // namespace fv_converter
namespace regression {
class regression_base;
}  // namespace regression
namespace driver {

class regression : public driver_base {
 public:
  typedef core::regression::regression_base regression_base;
  regression(
      jubatus::util::lang::shared_ptr<regression_base>
          regression_method,
      jubatus::util::lang::shared_ptr<fv_converter::datum_to_fv_converter>
          converter);
  virtual ~regression();

  void train(const std::pair<double, fv_converter::datum>& data);
  double estimate(const fv_converter::datum& data) const;

  void get_status(std::map<std::string, std::string>& status) const;
  void clear();

  void pack(framework::packer& pk) const;
  void unpack(msgpack::object o);

 private:
  jubatus::util::lang::shared_ptr<fv_converter::datum_to_fv_converter>
    converter_;
  jubatus::util::lang::shared_ptr<jubatus::core::regression::regression_base>
    regression_;
  fv_converter::mixable_weight_manager wm_;
};

}  // namespace driver
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_DRIVER_REGRESSION_HPP_
