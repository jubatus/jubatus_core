// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2015 Preferred Infrastructure and Nippon Telegraph and Telephone Corporation.
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

#ifndef JUBATUS_CORE_COMMON_EXPORT_MODEL_HPP__
#define JUBATUS_CORE_COMMON_EXPORT_MODEL_HPP__

#include <msgpack.hpp>
#include "../framework/packer.hpp"

#define JUBATUS_EXPORT_MODEL(...)                                   \
  void export_model(framework::packer& pk) const {                  \
    msgpack::type::make_define(__VA_ARGS__).msgpack_pack(pk);       \
  }
#define JUBATUS_IMPORT_MODEL(...)                                   \
  void import_model(msgpack::object o) {                            \
    this->clear();                                                  \
    msgpack::type::make_define(__VA_ARGS__).msgpack_unpack(o);      \
  }

#define JUBATUS_PORTING_MODEL(...)              \
  JUBATUS_IMPORT_MODEL(__VA_ARGS__)             \
  JUBATUS_EXPORT_MODEL(__VA_ARGS__)
#endif  // JUBATUS_CORE_COMMON_EXPORT_MODEL_HPP_
