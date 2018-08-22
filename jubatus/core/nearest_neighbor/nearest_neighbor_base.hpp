// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2012 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#ifndef JUBATUS_CORE_NEAREST_NEIGHBOR_NEAREST_NEIGHBOR_BASE_HPP_
#define JUBATUS_CORE_NEAREST_NEIGHBOR_NEAREST_NEIGHBOR_BASE_HPP_

#include <stdint.h>
#include <iosfwd>
#include <string>
#include <utility>
#include <vector>
#include "jubatus/util/lang/shared_ptr.h"
#include "../common/type.hpp"
#include "../framework/mixable_versioned_table.hpp"
#include "../framework/mixable.hpp"

namespace jubatus {
namespace core {
namespace storage {
class column_table;
}  // namespace storage
namespace nearest_neighbor {

class nearest_neighbor_base {
 public:
  explicit nearest_neighbor_base(
      jubatus::util::lang::shared_ptr<storage::column_table> table,
      const std::string& id);
  virtual ~nearest_neighbor_base() {}

  void get_all_row_ids(std::vector<std::string>& ids) const;
  uint64_t size() const;

  jubatus::util::lang::shared_ptr<const storage::column_table>
  get_const_table() const {
    return mixable_table_->get_model();
  }
  jubatus::util::lang::shared_ptr<storage::column_table> get_table() {
    return mixable_table_->get_model();
  }

  virtual std::string type() const = 0;

  virtual void clear();

  virtual void set_row(const std::string& id, const common::sfv_t& sfv) = 0;
  virtual void neighbor_row(
      const common::sfv_t& query,
      std::vector<std::pair<std::string, double> >& ids,
      uint64_t ret_num) const = 0;
  virtual void neighbor_row(
      const std::string& query_id,
      std::vector<std::pair<std::string, double> >& ids,
      uint64_t ret_num) const = 0;
  virtual double calc_similarity(double distance) const {
    return 1 - distance;
  }

  virtual void similar_row(
      const common::sfv_t& query,
      std::vector<std::pair<std::string, double> >& ids,
      uint64_t ret_num) const;
  virtual void similar_row(
      const std::string& query_id,
      std::vector<std::pair<std::string, double> >& ids,
      uint64_t ret_num) const;

  void pack(framework::packer& packer) const;
  void unpack(msgpack::object o);

  framework::mixable* get_mixable() const;

 protected:
  std::string my_id_;

 private:
  jubatus::util::lang::shared_ptr<framework::mixable_versioned_table>
      mixable_table_;
};

}  // namespace nearest_neighbor
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_NEAREST_NEIGHBOR_NEAREST_NEIGHBOR_BASE_HPP_
