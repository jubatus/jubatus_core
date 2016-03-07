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

#ifndef JUBATUS_CORE_STORAGE_INVERTED_INDEX_STORAGE_HPP_
#define JUBATUS_CORE_STORAGE_INVERTED_INDEX_STORAGE_HPP_

#include <string>
#include <utility>
#include <vector>
#include <msgpack.hpp>
#include "jubatus/util/data/unordered_map.h"
#include "jubatus/util/lang/function.h"
#include "jubatus/util/lang/bind.h"
#include "storage_type.hpp"
#include "../common/version.hpp"
#include "../common/type.hpp"
#include "../common/unordered_map.hpp"
#include "../common/key_manager.hpp"
#include "../framework/mixable_helper.hpp"
#include "../unlearner/unlearner_base.hpp"
#include "sparse_matrix_storage.hpp"

using jubatus::util::lang::function;
using jubatus::util::lang::bind;
using jubatus::util::lang::_1;

namespace jubatus {
namespace core {
namespace storage {

class inverted_index_storage {
 public:
  struct diff_type {
    sparse_matrix_storage inv;
    map_float_t column2norm;

    MSGPACK_DEFINE(inv, column2norm);
  };

  inverted_index_storage();
  ~inverted_index_storage();
  void set_unlearner(
      util::lang::shared_ptr<unlearner::unlearner_base> unlearner) {
    unlearner_ = unlearner;
  }

  void set(const std::string& row, const std::string& column, float val);
  float get(const std::string& row, const std::string& column) const;
  void remove(const std::string& row, const std::string& column);
  void remove_row(const std::string& row);
  void clear();
  void get_all_column_ids(std::vector<std::string>& ids) const;

  void calc_scores(
      const common::sfv_t& sfv,
      std::vector<std::pair<std::string, float> >& scores,
      size_t ret_num) const;

  void calc_euclid_scores(
      const common::sfv_t& sfv,
      std::vector<std::pair<std::string, float> >& scores,
      size_t ret_num) const;

  void get_diff(diff_type& diff_str) const;
  bool put_diff(const diff_type& mixed_diff);
  void mix(const diff_type& lhs_str, diff_type& rhs_str) const;

  storage::version get_version() const {
    return storage::version();
  }

  std::string name() const;

  void pack(framework::packer& packer) const;
  void unpack(msgpack::object o);

  MSGPACK_DEFINE(inv_, inv_diff_, column2norm_, column2norm_diff_, column2id_);

 private:
  static float calc_l2norm(const common::sfv_t& sfv);
  float calc_columnl2norm(uint64_t column_id) const;
  float get_from_tbl(
      const std::string& row,
      uint64_t column_id,
      const tbl_t& tbl,
      bool& exist) const;

  void add_inp_scores(
      const std::string& row,
      float val,
      std::vector<float>& scores) const;

  tbl_t inv_;
  tbl_t inv_diff_;
  imap_float_t column2norm_;
  imap_float_t column2norm_diff_;
  common::key_manager column2id_;
  util::lang::shared_ptr<unlearner::unlearner_base> unlearner_;
};

typedef framework::linear_mixable_helper<
    inverted_index_storage, inverted_index_storage::diff_type>
    mixable_inverted_index_storage;

}  // namespace storage
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_STORAGE_INVERTED_INDEX_STORAGE_HPP_
