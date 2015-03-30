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

#include "mixable_versioned_table.hpp"

#include <algorithm>
#include <string>
#include <vector>
#include <memory>
#include <msgpack.hpp>
#include "../common/exception.hpp"
#include "../common/byte_buffer.hpp"
#include "../framework/packer.hpp"
#include "../framework/stream_writer.hpp"

typedef jubatus::core::storage::column_table::version_t version_t;

using jubatus::util::lang::shared_ptr;
using jubatus::core::framework::diff_object;
using jubatus::core::framework::diff_object_raw;
using jubatus::core::framework::stream_writer;
using std::vector;

namespace jubatus {
namespace core {
namespace framework {
namespace {

struct internal_diff : framework::diff_object_raw {
  void convert_binary(framework::packer& pk) const {
    msgpack::sbuffer data;
    core::framework::stream_writer<msgpack::sbuffer> st(data);
    core::framework::jubatus_packer jp(st);
    core::framework::packer p(jp);
    p.pack(objs);

    pk.pack_raw(data.size());
    pk.pack_raw_body(data.data(), data.size());
  }

  vector<msgpack::object> objs;
  vector<shared_ptr<msgpack::zone> > zones;
};

}  // namespace

framework::diff_object mixable_versioned_table::convert_diff_object(
    const msgpack::object& obj) const {
  if (obj.type != msgpack::type::RAW) {
    throw JUBATUS_EXCEPTION(
        core::common::exception::runtime_error("bad diff_object"));
  }
  internal_diff* diff = new internal_diff;
  diff_object diff_obj(diff);

  msgpack::unpacked msg;
  msgpack::unpack(&msg, obj.via.raw.ptr, obj.via.raw.size);

  msg.get().convert(&diff->objs);
  if (!diff->objs.empty()) {
    shared_ptr<msgpack::zone> owner(msg.zone().release());
    diff->zones.push_back(owner);
  }
  return diff_obj;
}

void mixable_versioned_table::get_diff(framework::packer& pk) const {
  msgpack::sbuffer data;
  core::framework::stream_writer<msgpack::sbuffer> st(data);
  core::framework::jubatus_packer jp(st);
  core::framework::packer p(jp);
  pull_impl(vc_, p);

  // Wrap msgpack binary more for holding msgpack::zone in internal diff_object.
  pk.pack_raw(data.size());
  pk.pack_raw_body(data.data(), data.size());
}

bool mixable_versioned_table::put_diff(const framework::diff_object& ptr) {
  internal_diff* diff_obj = dynamic_cast<internal_diff*>(ptr.get());
  if (!diff_obj) {
    throw JUBATUS_EXCEPTION(
        core::common::exception::runtime_error("bad diff_object"));
  }

  msgpack::object obj;
  obj.type = msgpack::type::ARRAY;
  obj.via.array.ptr = &diff_obj->objs[0];
  obj.via.array.size = diff_obj->objs.size();

  push_impl(obj);

  return true;
}

void mixable_versioned_table::mix(
    const msgpack::object& obj,
    framework::diff_object ptr) const {
  internal_diff* diff_obj = dynamic_cast<internal_diff*>(ptr.get());
  if (!diff_obj) {
    throw JUBATUS_EXCEPTION(
        core::common::exception::runtime_error("bad diff_object"));
  }

  if (obj.type != msgpack::type::RAW) {
    throw JUBATUS_EXCEPTION(
        core::common::exception::runtime_error("bad diff_object"));
  }
  msgpack::unpacked msg;
  msgpack::unpack(&msg, obj.via.raw.ptr, obj.via.raw.size);
  msgpack::object o = msg.get();
  if (o.type != msgpack::type::ARRAY) {
    throw JUBATUS_EXCEPTION(
        core::common::exception::runtime_error("bad diff_object"));
  }
  for (size_t i = 0; i < o.via.array.size; i++) {
    diff_obj->objs.push_back(o.via.array.ptr[i]);
  }

  if (o.via.array.size > 0) {
    shared_ptr<msgpack::zone> owner(msg.zone().release());
    diff_obj->zones.push_back(owner);
  }
}

void mixable_versioned_table::get_argument(framework::packer& pk) const {
  pk.pack(vc_);
}

void mixable_versioned_table::pull(
    const msgpack::object& arg,
    framework::packer& pk) const {
  version_clock vc;
  arg.convert(&vc);
  pull_impl(vc, pk);
}

void mixable_versioned_table::push(const msgpack::object& diff) {
  push_impl(diff);
}

void mixable_versioned_table::pull_impl(
    const version_clock& vc, framework::packer& pk) const {

  model_ptr table = get_model();
  const uint64_t table_size = table->size();

  size_t pack_size = 0;
  for (uint64_t i = 0; i < table_size; ++i) {
    const version_t version = table->get_version(i);
    version_clock::const_iterator it = vc.find(version.first);
    if (it == vc.end() || it->second < version.second) {
      pack_size++;
    }
  }
  pk.pack_array(pack_size);

  for (uint64_t i = 0; i < table_size; ++i) {
    const version_t version = table->get_version(i);
    version_clock::const_iterator it = vc.find(version.first);
    if (it == vc.end() || it->second < version.second) {
      table->get_row(i, pk);
    }
  }
}

void mixable_versioned_table::push_impl(
    const msgpack::object& o) {
  model_ptr table = get_model();
  if (o.type != msgpack::type::ARRAY) {
    throw JUBATUS_EXCEPTION(
        core::common::exception::runtime_error("bad diff_object"));
  }
  for (uint64_t i = 0; i < o.via.array.size; ++i) {
    const version_t version = table->set_row(o.via.array.ptr[i]);
    update_version(version);
  }
}

void mixable_versioned_table::update_version(const version_t& version) {
  version_clock::iterator it = vc_.find(version.first);
  if (it == vc_.end()) {
    vc_.insert(version);
  } else if (it->second < version.second) {
    it->second = version.second;
  }
}

}  // namespace framework
}  // namespace core
}  // namespace jubatus
