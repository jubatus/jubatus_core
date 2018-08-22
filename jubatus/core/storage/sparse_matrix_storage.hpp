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

#ifndef JUBATUS_CORE_STORAGE_SPARSE_MATRIX_STORAGE_HPP_
#define JUBATUS_CORE_STORAGE_SPARSE_MATRIX_STORAGE_HPP_

#include <string>
#include <utility>
#include <vector>
#include <msgpack.hpp>
#include "jubatus/util/data/unordered_map.h"
#include "../common/key_manager.hpp"
#include "../common/unordered_map.hpp"
#include "../framework/model.hpp"
#include "storage_type.hpp"

namespace jubatus {
namespace core {
namespace storage {

class sparse_matrix_storage : public framework::model {
 public:
  sparse_matrix_storage();
  ~sparse_matrix_storage();

  sparse_matrix_storage& operator =(const sparse_matrix_storage&);

  void set(const std::string& row, const std::string& column, double val);
  void set_row(
      const std::string& row,
      const std::vector<std::pair<std::string, double> >& columns);

  double get(const std::string& row, const std::string& column) const;
  void get_row(
      const std::string& row,
      std::vector<std::pair<std::string, double> >& columns) const;

  double calc_l2norm(const std::string& row) const;
  void remove(const std::string& row, const std::string& column);
  void remove_row(const std::string& row);
  void get_all_row_ids(std::vector<std::string>& ids) const;
  void clear();

  storage::version get_version() const {
    return storage::version();
  }

  void pack(framework::packer& packer) const;
  void unpack(msgpack::object o);

  friend std::ostream& operator<<(std::ostream& o,
                                  const sparse_matrix_storage& s) {
    for (tbl_t::const_iterator it = s.tbl_.begin();
         it != s.tbl_.end();
         ++it) {
      o << "(" << it->first << "): ";
      for (row_t::const_iterator jt = it->second.begin();
           jt != it->second.end();
           ++jt) {
        o << s.column2id_.get_key(jt->first) << "->" << jt->second << ", ";
      }
    }
    return o;
  }

 private:
  tbl_t tbl_;
  common::key_manager column2id_;
  storage::version version_;

 public:
  MSGPACK_DEFINE(tbl_, column2id_);
};

}  // namespace storage
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_STORAGE_SPARSE_MATRIX_STORAGE_HPP_
