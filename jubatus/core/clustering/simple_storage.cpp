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

#include "simple_storage.hpp"

#include <string>
#include <vector>

namespace jubatus {
namespace core {
namespace clustering {

simple_storage::simple_storage(
    const std::string& name,
    const int bucket_size)
  :  storage(name), bucket_size_(bucket_size) {
    if (!(2 <= bucket_size)) {
      throw JUBATUS_EXCEPTION(
          common::invalid_parameter("2 <= bucket_size"));
    }
}

void simple_storage::add(const weighted_point& point) {
  static size_t cnt = 0;
  ++cnt;
  if (cnt % bucket_size_ == 0) {
    increment_revision();
  }
  mine_.push_back(point);
}

wplist simple_storage::get_mine() const {
  return mine_;
}

void simple_storage::pack_impl_(framework::packer& packer) const {
  packer.pack_array(2);
  storage::pack_impl_(packer);
  packer.pack(mine_);
}

void simple_storage::unpack_impl_(msgpack::object o) {
  std::vector<msgpack::object> mems;
  o.convert(&mems);
  if (mems.size() != 2) {
    throw msgpack::type_error();
  }
  storage::unpack_impl_(mems[0]);
  mems[1].convert(&mine_);
}

void simple_storage::clear_impl_() {
  storage::clear_impl_();
  mine_.clear();
}

}  // namespace clustering
}  // namespace core
}  // namespace jubatus
