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

#include "random_unlearner.hpp"

#include <string>
#include <limits>
#include "../common/exception.hpp"
#include "../common/assert.hpp"

namespace jubatus {
namespace core {
namespace unlearner {

const random_unlearner::random_unlearner_config&
as_random_unlearner_config(const unlearner_config_base& orig) {
  return dynamic_cast<const random_unlearner::random_unlearner_config&>(orig);
}

random_unlearner::random_unlearner(const unlearner_config_base& conf)
    : max_size_(as_random_unlearner_config(conf).max_size) {
  const random_unlearner_config& rconf = as_random_unlearner_config(conf);
  if (rconf.max_size <= 0) {
    throw JUBATUS_EXCEPTION(
        common::config_exception() << common::exception::error_message(
            "max_size must be a positive integer"));
  }
  if (rconf.seed) {
    if (*rconf.seed < 0 || std::numeric_limits<uint32_t>::max() < *rconf.seed) {
      throw JUBATUS_EXCEPTION(
          common::config_exception() << common::exception::error_message(
              "unlearner seed must be within unsigned 32 bit integer"));
    }
    mtr_ = jubatus::util::math::random::mtrand(*rconf.seed);
  }
  id_map_.reserve(max_size_);
  ids_.reserve(max_size_);
}

bool random_unlearner::can_touch(const std::string& id) {
  return true;
}

bool random_unlearner::touch(const std::string& id) {
  if (id_map_.count(id) > 0) {
    return true;
  }

  size_t new_id_pos = -1;
  if (id_map_.size() < max_size_) {
    // Just add new ID to the ID set.
    ids_.push_back(id);
    new_id_pos = ids_.size() - 1;
  } else {
    // Need to unlearn the old entry and replace it with new one.
    new_id_pos = mtr_(id_map_.size());
    const std::string old_id = ids_[new_id_pos];
    unlearn(old_id);
    id_map_.erase(old_id);
    ids_[new_id_pos] = id;
  }
  id_map_.insert(std::make_pair(id, new_id_pos));

  return true;
}

bool random_unlearner::remove(const std::string& id) {
  if (id_map_.count(id) == 0) {
    return false;
  }

  const size_t id_pos = id_map_[id];
  id_map_.erase(id);

  // Overwrite the ID with the last element to avoid calling erase to vector.
  const std::string back_id = ids_.back();
  ids_.pop_back();
  if (id != back_id) {
    ids_[id_pos] = back_id;
    id_map_[back_id] = id_pos;
  }

  return true;
}

void random_unlearner::export_model(framework::packer& pk) const {
  pk.pack_array(1);  // [ids_]
  pk.pack(ids_);
}
void random_unlearner::import_model(msgpack::object o) {
    if(o.type != msgpack::type::ARRAY) {
    throw msgpack::type_error();
  }
  JUBATUS_ASSERT_EQ(1,
                    o.via.array.size,
                    "importing random_unlearner length must be 1");
  o.via.array.ptr[0].convert(&ids_);
  rebuild_map();
}

void random_unlearner::rebuild_map() {
  id_map_.clear();
  for (size_t i = 0; i < ids_.size(); ++i) {
    id_map_[ids_[i]] = i;
  }
}

bool random_unlearner::exists_in_memory(const std::string& id) const {
  return id_map_.count(id) > 0;
}

}  // namespace unlearner
}  // namespace core
}  // namespace jubatus
