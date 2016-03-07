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

#ifndef JUBATUS_CORE_STORAGE_STORAGE_BASE_HPP_
#define JUBATUS_CORE_STORAGE_STORAGE_BASE_HPP_

#include <map>
#include <string>
#include <utility>
#include <stdexcept>
#include <vector>
#include "jubatus/util/concurrent/rwmutex.h"
#include "jubatus/util/lang/shared_ptr.h"
#include "storage_type.hpp"
#include "storage_exception.hpp"
#include "../common/version.hpp"
#include "../common/exception.hpp"
#include "../common/type.hpp"
#include "../framework/model.hpp"

namespace jubatus {
namespace core {
namespace storage {

class storage_base : public framework::model {
 public:
  virtual ~storage_base() {
  }

  virtual void get(const std::string& feature, feature_val1_t& ret) const = 0;
  virtual void get_nolock(const std::string& feature,
                          feature_val1_t& ret) const = 0;
  virtual void get2(const std::string& feature, feature_val2_t& ret) const = 0;
  virtual void get2_nolock(const std::string& feature,
                           feature_val2_t& ret) const = 0;
  virtual void get3(const std::string& feature, feature_val3_t& ret) const = 0;
  virtual void get3_nolock(const std::string& feature,
                           feature_val3_t& ret) const = 0;

  // inner product
  virtual void inp(const common::sfv_t& sfv, map_feature_val1_t& ret) const = 0;

  virtual void set(
      const std::string& feature,
      const std::string& klass,
      const val1_t& w) = 0;
  virtual void set_nolock(
      const std::string& feature,
      const std::string& klass,
      const val1_t& w) = 0;
  virtual void set2(
      const std::string& feature,
      const std::string& klass,
      const val2_t& w) = 0;
  virtual void set2_nolock(
      const std::string& feature,
      const std::string& klass,
      const val2_t& w) = 0;
  virtual void set3(
      const std::string& feature,
      const std::string& klass,
      const val3_t& w) = 0;
  virtual void set3_nolock(
      const std::string& feature,
      const std::string& klass,
      const val3_t& w) = 0;

  virtual void get_status(std::map<std::string, std::string>&) const = 0;

  virtual void pack(framework::packer& packer) const = 0;
  virtual void unpack(msgpack::object o) = 0;

  virtual version get_version() const = 0;

  virtual void update(
      const std::string& feature,
      const std::string& inc_class,
      const std::string& dec_class,
      const val1_t& w);

  virtual void bulk_update(
      const common::sfv_t& sfv,
      float step_width,
      const std::string& inc_class,
      const std::string& dec_class);

  virtual util::concurrent::rw_mutex& get_lock() const = 0;

  virtual void get_diff(diff_t&) const;
  virtual bool set_average_and_clear_diff(const diff_t&);

  virtual void register_label(const std::string& label) = 0;

  virtual void clear() = 0;

  virtual std::vector<std::string> get_labels() const = 0;
  virtual bool set_label(const std::string& label) = 0;
  virtual bool delete_label(const std::string& label) = 0;
  virtual bool delete_label_nolock(const std::string& label) = 0;

  virtual std::string type() const = 0;
};

}  // namespace storage

typedef jubatus::util::lang::shared_ptr<jubatus::core::storage::storage_base>
  storage_ptr;

}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_STORAGE_STORAGE_BASE_HPP_
