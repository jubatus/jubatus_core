// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2011 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#ifndef JUBATUS_CORE_COMMON_KEY_MANAGER_HPP_
#define JUBATUS_CORE_COMMON_KEY_MANAGER_HPP_

#include <stdint.h>

#include <algorithm>
#include <string>
#include <vector>

#include <msgpack.hpp>
#include "jubatus/util/data/unordered_map.h"
#include "unordered_map.hpp"

namespace jubatus {
namespace core {
namespace common {

class key_manager {
 public:
  enum {
    NOTFOUND = 0xFFFFFFFFFFFFFFFFLLU
  };

  key_manager();
  // following member funcions are implicitly defined:
  //   key_manager(const key_manager& k) = default;
  //   key_manager& operator=(const key_manager& k) = default;
  //   ~key_manager() = default;
  void swap(key_manager& km) {
    key2id_.swap(km.key2id_);
    id2key_.swap(km.id2key_);
    std::swap(next_id_, km.next_id_);
  }

  size_t size() const {
    return key2id_.size();
  }

  uint64_t get_id(const std::string& key);
  uint64_t get_id_const(const std::string& key) const;
  const std::string& get_key(const uint64_t id) const;
  std::vector<std::string> get_all_id2key() const;
  void clear();
  bool set_key(const std::string& key);

  void init_by_id2key(const std::vector<std::string>& id2key);
  void delete_key(const std::string& name);

  uint64_t get_max_id() const;

  friend std::ostream& operator<<(std::ostream& os, const key_manager& km) {
    typedef jubatus::util::data::unordered_map<std::string, uint64_t> key2id_t;
    os << "[";
    for (key2id_t::const_iterator it = km.key2id_.begin();
         it != km.key2id_.end();
         ++it) {
      os << it->first << ":" << it->second << ", ";
    }
    os << "]";
    return os;
  }
  MSGPACK_DEFINE(key2id_, id2key_, next_id_);

 private:
  uint64_t append_key(const std::string& key);

  uint64_t next_id_;
  util::data::unordered_map<std::string, uint64_t> key2id_;
  util::data::unordered_map<uint64_t, std::string> id2key_;
};

inline void swap(key_manager& l, key_manager& r) {  // NOLINT
  l.swap(r);
}

}  // namespace common
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_COMMON_KEY_MANAGER_HPP_
