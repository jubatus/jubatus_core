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

#ifndef JUBATUS_CORE_STORAGE_BIT_INDEX_STORAGE_HPP_
#define JUBATUS_CORE_STORAGE_BIT_INDEX_STORAGE_HPP_

#include <string>
#include <utility>
#include <vector>
#include "jubatus/util/data/unordered_map.h"
#include "jubatus/util/lang/shared_ptr.h"
#include "../common/key_manager.hpp"
#include "../common/unordered_map.hpp"
#include "../framework/mixable_helper.hpp"
#include "storage_type.hpp"
#include "sparse_matrix_storage.hpp"
#include "bit_vector.hpp"
#include "../unlearner/unlearner_base.hpp"

namespace jubatus {
namespace core {
namespace storage {

class bit_index_storage : public framework::model {
 public:
  bit_index_storage();
  ~bit_index_storage();

  void set_unlearner(
      util::lang::shared_ptr<unlearner::unlearner_base> unlearner) {
    unlearner_ = unlearner;
  }

  void set_row(const std::string& row, const bit_vector& bv);
  void get_row(const std::string& row, bit_vector& bv) const;
  void remove_row(const std::string& row);
  void clear();
  void get_all_row_ids(std::vector<std::string>& ids) const;

  void similar_row(
      const bit_vector& bv,
      std::vector<std::pair<std::string, float> >& ids,
      uint64_t ret_num) const;
  std::string name() const;
  storage::version get_version() const {
    return storage::version();
  }

  void pack(framework::packer& packer) const;
  void unpack(msgpack::object o);

  void get_diff(bit_table_t& diff) const;
  bool put_diff(const bit_table_t& mixed_diff);
  void mix(const bit_table_t& lhs, bit_table_t& rhs) const;

  MSGPACK_DEFINE(bitvals_, bitvals_diff_);

 private:
  bit_table_t bitvals_;
  bit_table_t bitvals_diff_;
  util::lang::shared_ptr<unlearner::unlearner_base> unlearner_;
};

typedef framework::linear_mixable_helper<bit_index_storage, bit_table_t>
    mixable_bit_index_storage;

}  // namespace storage
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_STORAGE_BIT_INDEX_STORAGE_HPP_
