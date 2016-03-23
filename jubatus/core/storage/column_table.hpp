// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2012,2013 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#ifndef JUBATUS_CORE_STORAGE_COLUMN_TABLE_HPP_
#define JUBATUS_CORE_STORAGE_COLUMN_TABLE_HPP_

#include <stdint.h>
#include <algorithm>
#include <cstring>
#include <string>
#include <vector>
#include <utility>
#include <msgpack.hpp>

#include "jubatus/util/lang/cast.h"
#include "jubatus/util/lang/demangle.h"
#include "jubatus/util/data/unordered_map.h"
#include "jubatus/util/concurrent/rwmutex.h"
#include "jubatus/util/lang/shared_ptr.h"
#include "../common/assert.hpp"
#include "../common/exception.hpp"
#include "../common/unordered_map.hpp"
#include "../framework/packer.hpp"
#include "storage_exception.hpp"
#include "../unlearner/unlearner_base.hpp"
#include "bit_vector.hpp"
#include "column_type.hpp"
#include "abstract_column.hpp"
#include "owner.hpp"

namespace jubatus {
namespace core {
namespace storage {

class invalid_row_set
    : public common::exception::jubaexception<invalid_row_set> {
  const char* what() const throw() {
    return "invalid_row_set";
  }
};


class column_table {
  typedef jubatus::util::data::unordered_map<std::string, uint64_t> index_table;

 public:
  typedef std::pair<owner, uint64_t> version_t;

  column_table()
      : tuples_(0), clock_(0) {
  }
  ~column_table() {
  }

  void init(const std::vector<column_type>& schema);
  void clear();

  template<typename T1>
  bool add(const std::string& key, const owner& o, const T1& v1) {
    if (columns_.size() != 1) {
      throw length_unmatch_exception(
          "tuple's length unmatch, expected " +
          jubatus::util::lang::lexical_cast<std::string>(tuples_) + " tuples.");
    }
    // check already exists
    jubatus::util::concurrent::scoped_wlock lk(table_lock_);
    index_table::const_iterator it = index_.find(key);
    const bool not_found = it == index_.end();
    if (not_found) {
      // add tuple
      keys_.push_back(key);
      versions_.push_back(std::make_pair(o, clock_));
      columns_[0].push_back(v1);
      JUBATUS_ASSERT_EQ(keys_.size(), versions_.size(), "");

      // make index
      index_.insert(std::make_pair(key, tuples_));
      ++tuples_;
    } else {  // key exists
      const uint64_t index = it->second;
      versions_[index] = std::make_pair(o, clock_);
      columns_[0].update(index, v1);
    }
    ++clock_;
    return not_found;
  }

  template<typename T1, typename T2>
  bool add(const std::string& key, const owner& o, const T1& v1, const T2& v2) {
    if (columns_.size() != 2) {
      throw length_unmatch_exception(
          "tuple's length unmatch, expected " +
          jubatus::util::lang::lexical_cast<std::string>(tuples_) + " tuples.");
    }

    // check already exists */
    jubatus::util::concurrent::scoped_wlock lk(table_lock_);
    index_table::const_iterator it = index_.find(key);
    const bool not_found = it == index_.end();
    if (not_found) {
      // add tuple
      keys_.push_back(key);
      versions_.push_back(std::make_pair(o , clock_));
      columns_[0].push_back(v1);
      columns_[1].push_back(v2);
      JUBATUS_ASSERT_EQ(keys_.size(), versions_.size(), "");

      // make index
      index_.insert(std::make_pair(key, tuples_));
      ++tuples_;
    } else {  // key exists
      const uint64_t index = it->second;
      versions_[index] = std::make_pair(o, clock_);
      columns_[0].update(index, v1);
      columns_[1].update(index, v2);
    }
    ++clock_;
    return not_found;
  }
  // more add() will be needed...

  template<typename T>
  bool update(
      const std::string& key,
      const owner& o,
      size_t column_id,
      const T& v) {
    jubatus::util::concurrent::scoped_wlock lk(table_lock_);
    index_table::iterator it = index_.find(key);
    if (tuples_ < column_id || it == index_.end()) {
      return false;
    }
    versions_[it->second] = std::make_pair(o, clock_);
    columns_[column_id].update(it->second, v);
    columns_[column_id].update(it->second, v);
    ++clock_;
    return true;
  }

  std::string get_key(uint64_t key_id) const {
    jubatus::util::concurrent::scoped_rlock lk(table_lock_);
    return get_key_nolock(key_id);
  }

  std::string get_key_nolock(uint64_t key_id) const {
    if (tuples_ <= key_id) {
      return "";
    }
    return keys_[key_id];
  }

  void scan_clock() {
    jubatus::util::concurrent::scoped_wlock lk(table_lock_);
    uint64_t max_clock = 0;
    for (std::vector<version_t>::const_iterator it = versions_.begin();
         it != versions_.end(); ++it) {
      max_clock = std::max(max_clock, it->second);
    }
    clock_ = max_clock;
  }

  /* get_column methods
     ex. get_int8_column(), get_float_column(), get_bit_vector_column()...
     argument is column_id
     if type unmatched, it throws type_unmatch_exception
  */
  uint8_column& get_uint8_column(size_t column_id);
  uint16_column& get_uint16_column(size_t column_id);
  uint32_column& get_uint32_column(size_t column_id);
  uint64_column& get_uint64_column(size_t column_id);
  int8_column& get_int8_column(size_t column_id);
  int16_column& get_int16_column(size_t column_id);
  int32_column& get_int32_column(size_t column_id);
  int64_column& get_int64_column(size_t column_id);
  float_column& get_float_column(size_t column_id);
  double_column& get_double_column(size_t column_id);
  string_column& get_string_column(size_t column_id);
  bit_vector_column& get_bit_vector_column(size_t column_id);

  const_uint8_column& get_uint8_column(size_t column_id) const;
  const_uint16_column& get_uint16_column(size_t column_id) const;
  const_uint32_column& get_uint32_column(size_t column_id) const;
  const_uint64_column& get_uint64_column(size_t column_id) const;
  const_int8_column& get_int8_column(size_t column_id) const;
  const_int16_column& get_int16_column(size_t column_id) const;
  const_int32_column& get_int32_column(size_t column_id) const;
  const_int64_column& get_int64_column(size_t column_id) const;
  const_float_column& get_float_column(size_t column_id) const;
  const_double_column& get_double_column(size_t column_id) const;
  const_string_column& get_string_column(size_t column_id) const;
  const_bit_vector_column& get_bit_vector_column(size_t column_id) const;

  uint64_t size() const {
    jubatus::util::concurrent::scoped_rlock lk(table_lock_);
    return size_nolock();
  }

  uint64_t size_nolock() const {
    return tuples_;
  }

  std::string dump_json() const {
    jubatus::util::concurrent::scoped_rlock lk(table_lock_);
    std::stringstream ss;
    ss << tuples_;
    return ss.str();
  }

  std::pair<bool, uint64_t> exact_match(const std::string& prefix) const;
  std::pair<bool, uint64_t> exact_match_nolock(
      const std::string& prefix) const;

  friend std::ostream& operator<<(std::ostream& os, const column_table& tbl) {
    jubatus::util::concurrent::scoped_rlock lk(tbl.table_lock_);
    os << "total size:" << tbl.tuples_ << std::endl;
    os << "types: vesions|";
    for (size_t j = 0; j < tbl.columns_.size(); ++j) {
      os << tbl.columns_[j].type().type_as_string() << "\t|";
    }
    os << std::endl;
    for (uint64_t i = 0; i < tbl.tuples_; ++i) {
      os << tbl.keys_[i] << ":" <<
          tbl.versions_[i].first << ":" << tbl.versions_[i].second << "\t|";
      for (size_t j = 0; j < tbl.columns_.size(); ++j) {
        tbl.columns_[j].dump(os, i);
        os << "\t|";
      }
      os << std::endl;
    }
    return os;
  }

  version_t get_version(uint64_t index) const {
    jubatus::util::concurrent::scoped_rlock lk(table_lock_);
    return versions_[index];
  }

  void get_row(const uint64_t id, framework::packer& pk) const {
    jubatus::util::concurrent::scoped_rlock lk(table_lock_);
    JUBATUS_ASSERT_GE(tuples_, id, "specified index is bigger than table size");
    pk.pack_array(3);  // [key, [owner, id], [data]]
    pk.pack(keys_[id]);  // key
    pk.pack(versions_[id]);  // [version]
    pk.pack_array(columns_.size());
    for (size_t i = 0; i < columns_.size(); ++i) {
      columns_[i].pack_with_index(id, pk);
    }
  }

  std::vector<column_type> types() const {
    std::vector<column_type> ret;
    ret.reserve(columns_.size());
    for (size_t i = 0; i < columns_.size(); ++i) {
      ret.push_back(columns_[i].type());
    }
    return ret;
  }

  version_t set_row(const msgpack::object& o,
                    unlearner::unlearner_base* unlearner = NULL) {
    if (o.type != msgpack::type::ARRAY || o.via.array.size != 3) {
      throw msgpack::type_error();
    }

    const std::string& key = o.via.array.ptr[0].as<std::string>();
    version_t set_version = o.via.array.ptr[1].as<version_t>();

    jubatus::util::concurrent::scoped_wlock lk(table_lock_);
    const msgpack::object& dat = o.via.array.ptr[2];
    index_table::iterator it = index_.find(key);
    if (unlearner) {
      unlearner->touch(key);
    }
    if (it == index_.end()) {  // did not exist, append
      if (dat.via.array.size != columns_.size()) {
        throw std::bad_cast();
      }

      // add tuple
      keys_.push_back(key);
      versions_.push_back(set_version);
      for (size_t i = 0; i < columns_.size(); ++i) {
        columns_[i].push_back(dat.via.array.ptr[i]);
      }
      JUBATUS_ASSERT_EQ(keys_.size(), versions_.size(), "");

      // make index
      index_.insert(std::make_pair(key, tuples_));
      ++tuples_;
    } else {  // already exist, overwrite if needed
      const uint64_t target = it->second;

      if (dat.via.array.size != columns_.size()) {
        throw std::bad_cast();
      }

      // overwrite tuple if needed
      if (versions_[target].second <= set_version.second) {
        // needed!!
        versions_[target] = set_version;
        for (size_t i = 0; i < columns_.size(); ++i) {
          columns_[i].update(target, dat.via.array.ptr[i]);
        }
        // make index
        index_.insert(std::make_pair(key, tuples_));
      }
    }
    if (clock_ <= set_version.second) {
      clock_ = set_version.second + 1;
    }
    return set_version;
  }

  bool update_clock(const std::string& target, const owner& o) {
    jubatus::util::concurrent::scoped_wlock lk(table_lock_);
    index_table::const_iterator it = index_.find(target);
    if (it == index_.end()) {
      return false;
    }
    versions_[it->second] = std::make_pair(o, clock_);
    ++clock_;
    return true;
  }

  bool update_clock(const uint64_t index, const owner& o) {
    jubatus::util::concurrent::scoped_wlock lk(table_lock_);
    if (size() < index) {
      return false;
    }
    versions_[index] = std::make_pair(o, clock_);
    ++clock_;
    return true;
  }

  version_t get_clock(const std::string& target) const {
    jubatus::util::concurrent::scoped_rlock lk(table_lock_);
    index_table::const_iterator it = index_.find(target);
    if (it == index_.end()) {
      return version_t();
    }
    return versions_[it->second];
  }

  version_t get_clock(const uint64_t index) const {
    jubatus::util::concurrent::scoped_rlock lk(table_lock_);
    if (size() < index) {
      return version_t();
    }
    return versions_[index];
  }

  bool delete_row(const std::string& target) {
    jubatus::util::concurrent::scoped_wlock lk(table_lock_);
    index_table::const_iterator it = index_.find(target);
    if (it == index_.end()) {
      return false;
    }
    delete_row_(it->second);
    return true;
  }

  bool delete_row(uint64_t index) {
    jubatus::util::concurrent::scoped_wlock lk(table_lock_);
    if (size() <= index) {
      return false;
    }
    delete_row_(index);
    return true;
  }

  util::concurrent::rw_mutex& get_mutex() const {
    return table_lock_;
  }

  MSGPACK_DEFINE(keys_, tuples_, versions_, columns_, clock_, index_);

  void pack(framework::packer& packer) const {
    jubatus::util::concurrent::scoped_rlock lk(table_lock_);
    packer.pack(*this);
  }

  void unpack(msgpack::object o) {
    jubatus::util::concurrent::scoped_wlock lk(table_lock_);
    o.convert(this);
  }

 private:
  std::vector<std::string> keys_;
  std::vector<version_t> versions_;
  std::vector<detail::abstract_column> columns_;
  mutable jubatus::util::concurrent::rw_mutex table_lock_;
  uint64_t tuples_;
  uint64_t clock_;
  index_table index_;

  void delete_row_(uint64_t index) {
    JUBATUS_ASSERT_LT(index, size(), "");

    for (std::vector<detail::abstract_column>::iterator jt = columns_.begin();
         jt != columns_.end();
         ++jt) {
      jt->remove(index);
    }
    {  // needs swap on last index
      index_table::iterator move_it = index_.find(keys_[tuples_ - 1]);
      move_it->second = index;
      index_.erase(keys_[index]);
    }

    if (index + 1 != keys_.size()) {
      std::swap(keys_[index], keys_.back());
    }
    keys_.pop_back();

    if (index + 1 != versions_.size()) {
      std::swap(versions_[index], versions_.back());
    }
    versions_.pop_back();

    --tuples_;
    ++clock_;

    JUBATUS_ASSERT_EQ(tuples_, index_.size(), "");
    JUBATUS_ASSERT_EQ(tuples_, keys_.size(), "");
    JUBATUS_ASSERT_EQ(tuples_, versions_.size(), "");
  }
};

}  // namespace storage
}  // namespcae core
}  // namespace jubatus

#endif  // JUBATUS_CORE_STORAGE_COLUMN_TABLE_HPP_
