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

#ifndef JUBATUS_CORE_DRIVER_WEIGHT_HPP_
#define JUBATUS_CORE_DRIVER_WEIGHT_HPP_

#include <stdint.h>

#include <string>
#include <map>

#include "driver.hpp"
#include "jubatus/util/lang/shared_ptr.h"
#include "../fv_converter/datum.hpp"
#include "../fv_converter/datum_to_fv_converter.hpp"
#include "../fv_converter/mixable_weight_manager.hpp"

using jubatus::util::lang::shared_ptr;

namespace jubatus {
namespace core {
namespace driver {

class weight : public driver_base {
 public:
  explicit weight(shared_ptr<fv_converter::datum_to_fv_converter>);
  virtual ~weight();

  common::sfv_t update(const fv_converter::datum&);
  common::sfv_t calc_weight(const fv_converter::datum&) const;

  void clear();
  void get_status(std::map<std::string, std::string>&) const;

  void pack(jubatus::core::framework::packer& pk) const;
  void unpack(msgpack::object o);

 private:
  shared_ptr<fv_converter::datum_to_fv_converter> converter_;
  fv_converter::mixable_weight_manager wm_;
};

}  // namespace driver
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_DRIVER_WEIGHT_HPP_
