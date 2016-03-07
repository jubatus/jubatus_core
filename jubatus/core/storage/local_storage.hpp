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

#ifndef JUBATUS_CORE_STORAGE_LOCAL_STORAGE_HPP_
#define JUBATUS_CORE_STORAGE_LOCAL_STORAGE_HPP_

#include <map>
#include <string>
#include <vector>
#include "jubatus/util/data/unordered_map.h"
#include "jubatus/util/concurrent/rwmutex.h"
#include "jubatus/util/concurrent/lock.h"
#include "storage_base.hpp"
#include "../common/key_manager.hpp"
#include "../common/version.hpp"

namespace jubatus {
namespace core {
namespace storage {

typedef jubatus::util::data::unordered_map<uint64_t, val3_t> id_feature_val3_t;
typedef jubatus::util::data::unordered_map<std::string, id_feature_val3_t>
  id_features3_t;

class local_storage : public storage_base {
 public:
  local_storage();
  ~local_storage();

  void get(const std::string &feature, feature_val1_t& ret) const;
  void get_nolock(const std::string &feature, feature_val1_t& ret) const;
  void get2(const std::string &feature, feature_val2_t& ret) const;
  void get2_nolock(const std::string &feature, feature_val2_t& ret) const;
  void get3(const std::string &feature, feature_val3_t& ret) const;
  void get3_nolock(const std::string &feature, feature_val3_t& ret) const;

  // inner product
  void inp(const common::sfv_t& sfv, map_feature_val1_t& ret) const;

  void set(
      const std::string& feature,
      const std::string& klass,
      const val1_t& w);
  void set_nolock(
      const std::string& feature,
      const std::string& klass,
      const val1_t& w);
  void set2(
      const std::string& feature,
      const std::string& klass,
      const val2_t& w);
  void set2_nolock(
      const std::string& feature,
      const std::string& klass,
      const val2_t& w);
  void set3(
      const std::string& feature,
      const std::string& klass,
      const val3_t& w);
  void set3_nolock(
      const std::string& feature,
      const std::string& klass,
      const val3_t& w);

  void get_status(std::map<std::string, std::string>& status) const;

  void update(
      const std::string& feature,
      const std::string& inc_class,
      const std::string& dec_class,
      const val1_t& v);
  void bulk_update(
      const common::sfv_t& sfv,
      float step_width,
      const std::string& inc_class,
      const std::string& dec_class);

  util::concurrent::rw_mutex& get_lock() const;

  void register_label(const std::string& label);
  bool delete_label(const std::string& label);
  bool delete_label_nolock(const std::string& label);

  void clear();
  std::vector<std::string> get_labels() const;
  bool set_label(const std::string& label);

  void pack(framework::packer& packer) const;
  void unpack(msgpack::object o);
  storage::version get_version() const {
    return storage::version();
  }
  std::string type() const;

  MSGPACK_DEFINE(tbl_, class2id_);

 private:
  // map_features3_t tbl_;
  mutable util::concurrent::rw_mutex mutex_;
  id_features3_t tbl_;
  common::key_manager class2id_;

  // used for dump data
  friend std::ostream& operator<<(std::ostream& os, const local_storage& ls) {
    util::concurrent::scoped_rlock lk(ls.mutex_);
    os << "{" << std::endl;
    for (id_features3_t::const_iterator it = ls.tbl_.begin();
         it != ls.tbl_.end();
         ++it) {
      os << "  {" << it->first << ": " << "{" << std::endl;
      const id_feature_val3_t val = it->second;
      for (id_feature_val3_t::const_iterator jt = val.begin();
           jt != val.end();
           ++jt) {
        os << "    " << ls.class2id_.get_key(jt->first) << ": " << jt->second;

        {
          id_feature_val3_t::const_iterator jt_next = jt;
          ++jt_next;
          if (jt_next != val.end()) {
            os << ", ";
          }
        }
        os << std::endl;
      }
      os << "  }";

      {
        id_features3_t::const_iterator it_next = it;
        ++it_next;
        if (it_next != ls.tbl_.end()) {
          os << ",";
        }
      }
      os << std::endl;
    }
    os << "}";
    return os;
  }
};

}  // namespace storage
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_STORAGE_LOCAL_STORAGE_HPP_
