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

#include "labels.hpp"

#include <string>

#include "../common/exception.hpp"

namespace jubatus {
namespace core {
namespace storage {

labels::labels() {
}

labels::~labels() {
}

data_t labels::get() const {
  return master_;
}

void labels::put(const data_t& data) {
  for (data_t::const_iterator it = data.begin(); it != data.end(); ++it) {
    master_.insert(*it);
  }
  version_.increment();
}

void labels::merge(const data_t& lhs, data_t& rhs) const {
  for (data_t::const_iterator it = lhs.begin(); it != lhs.end(); ++it) {
    rhs.insert(*it);
  }
}

void labels::pack(framework::packer& packer) const {
  packer.pack(*this);
}

void labels::unpack(msgpack::object o) {
  o.convert(this);
}

mixable_labels::mixable_labels(model_ptr model)
  : model_(model) {
}

mixable_labels::~mixable_labels() {
}

framework::diff_object mixable_labels::convert_diff_object(
    const msgpack::object& obj) const {
  internal_diff_object* diff = new internal_diff_object;
  framework::diff_object diff_obj(diff);
  obj.convert(&diff->diff_);
  return diff_obj;
}

void mixable_labels::mix(
    const msgpack::object& obj, framework::diff_object diff) const {
  internal_diff_object* diff_obj =
    dynamic_cast<internal_diff_object*>(diff.get());
  if (!diff_obj) {
    throw JUBATUS_EXCEPTION(
        common::exception::runtime_error("bad diff_object"));
  }

  data_t lhs;
  obj.convert(&lhs);

  model_->merge(lhs, diff_obj->diff_);
}

void mixable_labels::get_diff(framework::packer& packer) const {
  // Return all of the data.
  packer.pack(model_->get());
}

bool mixable_labels::put_diff(const framework::diff_object& diff) {
  internal_diff_object* diff_obj =
    dynamic_cast<internal_diff_object*>(diff.get());
  if (!diff_obj) {
    throw JUBATUS_EXCEPTION(
        common::exception::runtime_error("bad diff_object"));
  }

  model_->put(diff_obj->diff_);
  return true;
}

void mixable_labels::get_argument(framework::packer& packer) const {
  // Return dummy data.
  packer.pack(0);
}

void mixable_labels::pull(
    const msgpack::object& obj, framework::packer& packer) const {
  // Return all of the data.
  packer.pack(model_->get());
}

void mixable_labels::push(const msgpack::object& obj) {
  // Put all of the data.
  data_t data;
  obj.convert(&data);
  model_->put(data);
}

}  // namespace storage
}  // namespace core
}  // namespace jubatus
