// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2013 Preferred Infrastructure and Nippon Telegraph and Telephone Corporation.
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

#include "nearest_neighbor.hpp"

#include <string>
#include <utility>
#include <vector>
#include "../table/column/row_deleter.hpp"
#include "../fv_converter/weight_manager.hpp"
#include "../fv_converter/mixable_weight_manager.hpp"

namespace jubatus {
namespace core {
namespace driver {

using jubatus::util::lang::shared_ptr;
using fv_converter::mixable_weight_manager;
using fv_converter::weight_manager;

nearest_neighbor::nearest_neighbor(
    shared_ptr<core::nearest_neighbor::nearest_neighbor_base> nn,
    shared_ptr<fv_converter::datum_to_fv_converter> converter)
    : converter_(converter),
      nn_(nn),
      wm_(mixable_weight_manager::model_ptr(new weight_manager)) {
  register_mixable(nn_->get_mixable());
  register_mixable(&wm_);

  converter_->set_weight_manager(wm_.get_model());
}

nearest_neighbor::nearest_neighbor(
    shared_ptr<core::nearest_neighbor::nearest_neighbor_base> nn,
    shared_ptr<fv_converter::datum_to_fv_converter> converter,
    shared_ptr<unlearner::unlearner_base> unlearner)
    : converter_(converter),
      nn_(nn),
      unlearner_(unlearner) {
  register_mixable(nn_->get_mixable());
  unlearner->set_callback(table::row_deleter(nn_->get_table()));
}

void nearest_neighbor::set_row(
    const std::string& id,
    const fv_converter::datum& datum) {
  if (unlearner_) {
    unlearner_->touch(id);
  }
  common::sfv_t v;
  converter_->convert(datum, v);
  nn_->set_row(id, v);
}

std::vector<std::pair<std::string, float> >
nearest_neighbor::neighbor_row_from_id(const std::string& id, size_t size) {
  std::vector<std::pair<std::string, float> > ret;
  nn_->neighbor_row(id, ret, size);
  return ret;
}

std::vector<std::pair<std::string, float> >
nearest_neighbor::neighbor_row_from_datum(
    const fv_converter::datum& datum,
    size_t size) {
  common::sfv_t v;
  converter_->convert(datum, v);
  std::vector<std::pair<std::string, float> > ret;
  nn_->neighbor_row(v, ret, size);
  return ret;
}

std::vector<std::pair<std::string, float> >
nearest_neighbor::similar_row(const std::string& id, size_t ret_num) {
  std::vector<std::pair<std::string, float> > ret;
  nn_->similar_row(id, ret, ret_num);
  return ret;
}

std::vector<std::pair<std::string, float> >
nearest_neighbor::similar_row(
    const core::fv_converter::datum& datum,
    size_t ret_num) {
  common::sfv_t v;
  converter_->convert(datum, v);
  std::vector<std::pair<std::string, float> > ret;
  nn_->similar_row(v, ret, ret_num);
  return ret;
}

void nearest_neighbor::clear() {
  converter_->clear_weights();
  nn_->clear();
  if (unlearner_) {
    unlearner_->clear();
  }
}

void nearest_neighbor::pack(framework::packer& pk) const {
  nn_->pack(pk);
}

void nearest_neighbor::unpack(msgpack::object o) {
  nn_->clear();
  nn_->unpack(o);
}

}  // namespace driver
}  // namespace core
}  // namespace jubatus
