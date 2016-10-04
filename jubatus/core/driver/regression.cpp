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

#include "regression.hpp"

#include <map>
#include <string>
#include <utility>
#include <vector>
#include "../fv_converter/datum.hpp"
#include "../fv_converter/datum_to_fv_converter.hpp"
#include "../fv_converter/converter_config.hpp"
#include "../regression/regression_base.hpp"
#include "../storage/storage_factory.hpp"

using std::string;
using std::pair;
using jubatus::util::lang::shared_ptr;
using jubatus::core::fv_converter::weight_manager;
using jubatus::core::fv_converter::mixable_weight_manager;

namespace jubatus {
namespace core {
namespace driver {

regression::regression(
    shared_ptr<core::regression::regression_base> regression_method,
    shared_ptr<fv_converter::datum_to_fv_converter> converter)
    : converter_(converter)
    , regression_(regression_method)
    , wm_(mixable_weight_manager::model_ptr(new weight_manager)) {
  std::vector<framework::mixable*> mixables = regression_->get_mixables();
  for (size_t i = 0; i < mixables.size(); i++) {
    register_mixable(mixables[i]);
  }
  register_mixable(&wm_);

  converter_->set_weight_manager(wm_.get_model());
}

regression::~regression() {
}

void regression::train(const pair<float, fv_converter::datum>& data) {
  common::sfv_t v;
  converter_->convert_and_update_weight(data.second, v);
  regression_->train(v, data.first);
}

float regression::estimate(
    const fv_converter::datum& data) const {
  common::sfv_t v;
  converter_->convert(data, v);
  float value = regression_->estimate(v);
  return value;
}

void regression::get_status(std::map<string, string>& status) const {
  regression_->get_status(status);
}

void regression::clear() {
  regression_->clear();
  converter_->clear_weights();
}

void regression::pack(framework::packer& pk) const {
  pk.pack_array(2);
  regression_->pack(pk);
  wm_.get_model()->pack(pk);
}

void regression::unpack(msgpack::object o) {
  if (o.type != msgpack::type::ARRAY || o.via.array.size != 2) {
    throw msgpack::type_error();
  }

  // clear before load
  regression_->clear();
  converter_->clear_weights();
  regression_->unpack(o.via.array.ptr[0]);
  wm_.get_model()->unpack(o.via.array.ptr[1]);
}

}  // namespace driver
}  // namespace core
}  // namespace jubatus
