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

#include "anomaly.hpp"

#include <string>
#include <utility>
#include <vector>

#include "../anomaly/anomaly_factory.hpp"
#include "../anomaly/anomaly_base.hpp"
#include "../common/vector_util.hpp"
#include "../fv_converter/datum.hpp"
#include "../fv_converter/datum_to_fv_converter.hpp"
#include "../fv_converter/converter_config.hpp"
#include "../fv_converter/revert.hpp"
#include "../storage/storage_factory.hpp"

using std::string;
using std::vector;
using std::pair;
using jubatus::core::fv_converter::weight_manager;
using jubatus::core::fv_converter::mixable_weight_manager;
using jubatus::util::lang::shared_ptr;

namespace jubatus {
namespace core {
namespace driver {

anomaly::anomaly(
    shared_ptr<jubatus::core::anomaly::anomaly_base> anomaly_method,
    shared_ptr<fv_converter::datum_to_fv_converter> converter)
    : converter_(converter),
      anomaly_(anomaly_method),
      wm_(mixable_weight_manager::model_ptr(new weight_manager)) {
  vector<framework::mixable*> mixables = anomaly_->get_mixables();
  for (size_t i = 0; i < mixables.size(); i++) {
    register_mixable(mixables[i]);
  }
  register_mixable(&wm_);

  converter_->set_weight_manager(wm_.get_model());
}

anomaly::~anomaly() {
}

bool anomaly::is_updatable() const {
  return anomaly_->is_updatable();
}

void anomaly::clear_row(const std::string& id) {
  anomaly_->clear_row(id);
}

pair<string, float> anomaly::add(
    const string& id,
    const fv_converter::datum& d) {
  if (anomaly_->is_updatable()) {
    return make_pair(id, this->update(id, d));
  } else {
    return make_pair(id, this->overwrite(id, d));
  }
}

float anomaly::update(const string& id, const fv_converter::datum& d) {
  common::sfv_t v;
  converter_->convert_and_update_weight(d, v);

  if (anomaly_->update_row(id, v)) {
    return anomaly_->calc_anomaly_score(id);
  } else {
    return anomaly_->calc_anomaly_score(id, v);
  }
}

float anomaly::overwrite(const string& id, const fv_converter::datum& d) {
  common::sfv_t v;
  converter_->convert_and_update_weight(d, v);

  if (anomaly_->set_row(id, v)) {
    return anomaly_->calc_anomaly_score(id);
  } else {
    return anomaly_->calc_anomaly_score(v);
  }
}

void anomaly::clear() {
  anomaly_->clear();
  converter_->clear_weights();
}

float anomaly::calc_score(const fv_converter::datum& d) const {
  common::sfv_t v;
  converter_->convert(d, v);
  return anomaly_->calc_anomaly_score(v);
}

vector<string> anomaly::get_all_rows() const {
  vector<string> ids;
  anomaly_->get_all_row_ids(ids);
  return ids;
}

uint64_t anomaly::find_max_int_id() const {
  return anomaly_->find_max_int_id();
}

void anomaly::pack(framework::packer& pk) const {
  pk.pack_array(2);
  anomaly_->pack(pk);
  wm_.get_model()->pack(pk);
}

void anomaly::unpack(msgpack::object o) {
  if (o.type != msgpack::type::ARRAY || o.via.array.size != 2) {
    throw msgpack::type_error();
  }

  // clear before load
  anomaly_->clear();
  converter_->clear_weights();
  anomaly_->unpack(o.via.array.ptr[0]);
  wm_.get_model()->unpack(o.via.array.ptr[1]);
}

}  // namespace driver
}  // namespace core
}  // namespace jubatus
