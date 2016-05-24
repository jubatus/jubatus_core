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

#include "weight.hpp"

#include <string>
#include <map>

#include "jubatus/util/lang/shared_ptr.h"
#include "../common/vector_util.hpp"
#include "../fv_converter/datum.hpp"
#include "../fv_converter/datum_to_fv_converter.hpp"
#include "../fv_converter/mixable_weight_manager.hpp"

using std::string;
using jubatus::util::lang::shared_ptr;
using jubatus::core::fv_converter::weight_manager;
using jubatus::core::fv_converter::mixable_weight_manager;

namespace jubatus {
namespace core {
namespace driver {

weight::weight(
    shared_ptr<fv_converter::datum_to_fv_converter> converter)
    : converter_(converter)
    , wm_(mixable_weight_manager::model_ptr(new weight_manager)) {
  register_mixable(&wm_);

  converter_->set_weight_manager(wm_.get_model());
}

weight::~weight() {
}

common::sfv_t weight::update(const fv_converter::datum& d) {
  common::sfv_t v;
  converter_->convert_and_update_weight(d, v);
  common::sort_and_merge(v);
  return v;
}

common::sfv_t weight::calc_weight(const fv_converter::datum& d) const {
  common::sfv_t v;
  converter_->convert(d, v);
  common::sort_and_merge(v);
  return v;
}

void weight::clear() {
  converter_->clear_weights();
}

void weight::get_status(std::map<string, string>& status) const {
  wm_.get_model()->get_status(status);
}

void weight::pack(framework::packer& pk) const {
  pk.pack_array(1);
  wm_.get_model()->pack(pk);
}

void weight::unpack(msgpack::object o) {
  if (o.type != msgpack::type::ARRAY || o.via.array.size != 1) {
    throw msgpack::type_error();
  }

  // clear before load
  converter_->clear_weights();
  wm_.get_model()->unpack(o.via.array.ptr[0]);
}

}  // namespace driver
}  // namespace core
}  // namespace jubatus
