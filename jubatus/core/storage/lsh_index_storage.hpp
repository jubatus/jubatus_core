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

#ifndef JUBATUS_CORE_STORAGE_LSH_INDEX_STORAGE_HPP_
#define JUBATUS_CORE_STORAGE_LSH_INDEX_STORAGE_HPP_

#include <iosfwd>
#include <string>
#include <utility>
#include <vector>
#include <msgpack.hpp>
#include "jubatus/util/data/unordered_map.h"
#include "jubatus/util/data/unordered_set.h"
#include "lsh_vector.hpp"
#include "storage_type.hpp"
#include "../common/key_manager.hpp"
#include "../common/unordered_map.hpp"
#include "../framework/mixable_helper.hpp"
#include "../unlearner/unlearner_base.hpp"

namespace jubatus {
namespace core {
namespace storage {

struct lsh_entry {
  std::vector<uint64_t> lsh_hash;
  bit_vector simhash_bv;
  double norm;

  MSGPACK_DEFINE(lsh_hash, simhash_bv, norm);
};

typedef jubatus::util::data::unordered_map<std::string, lsh_entry>
  lsh_master_table_t;

typedef jubatus::util::data::unordered_map<uint64_t, std::vector<uint64_t> >
  lsh_table_t;

class lsh_index_storage {
 public:
  lsh_index_storage();
  lsh_index_storage(size_t lsh_num, size_t table_num, uint32_t seed);
  lsh_index_storage(size_t table_num, const std::vector<float>& shift);
  virtual ~lsh_index_storage();

  void set_unlearner(
      util::lang::shared_ptr<unlearner::unlearner_base> unlearner) {
    unlearner_ = unlearner;
  }

  // hash is a randomly-projected and scaled hash values without shifting
  void set_row(
      const std::string& row,
      const std::vector<float>& hash,
      double norm);
  void remove_row(const std::string& row);
  void clear();
  void get_all_row_ids(std::vector<std::string>& ids) const;

  void similar_row(
      const std::vector<float>& hash,
      double norm,
      uint64_t probe_num,
      uint64_t ret_num,
      std::vector<std::pair<std::string, double> >& ids) const;
  void similar_row(
      const std::string& id,
      uint64_t ret_num,
      std::vector<std::pair<std::string, double> >& ids) const;
  std::string name() const;

  size_t table_num() const {
    return table_num_;
  }

  size_t all_lsh_num() const {
    return shift_.size();
  }
  storage::version get_version() const {
    return storage::version();
  }

  void pack(framework::packer& packer) const;
  void unpack(msgpack::object o);

  void get_diff(lsh_master_table_t& diff) const;
  bool put_diff(const lsh_master_table_t& mixed_diff);
  void mix(const lsh_master_table_t& lhs, lsh_master_table_t& rhs) const;

  MSGPACK_DEFINE(master_table_, master_table_diff_, lsh_table_,
      lsh_table_diff_, shift_, table_num_, key_manager_);

 private:
  lsh_master_table_t::iterator remove_and_get_row(const std::string& row);
  void put_empty_entry(uint64_t row_id, const lsh_entry& entry);


  std::vector<float> make_entry(
      const std::vector<float>& hash,
      double norm,
      lsh_entry& entry) const;
  bool retrieve_hit_rows(
      uint64_t hash,
      size_t ret_num,
      jubatus::util::data::unordered_set<uint64_t>& cands) const;

  void get_sorted_similar_rows(
      const jubatus::util::data::unordered_set<uint64_t>& cands,
      const bit_vector& query_simhash,
      double query_norm,
      uint64_t ret_num,
      std::vector<std::pair<std::string, double> >& ids) const;
  const lsh_entry* get_lsh_entry(const std::string& row) const;
  void remove_model_row(const std::string& row);
  void set_mixed_row(const std::string& row, const lsh_entry& entry);

  lsh_master_table_t master_table_;
  lsh_master_table_t master_table_diff_;

  lsh_table_t lsh_table_;
  lsh_table_t lsh_table_diff_;

  std::vector<float> shift_;
  uint64_t table_num_;
  common::key_manager key_manager_;

  util::lang::shared_ptr<unlearner::unlearner_base> unlearner_;
};

typedef framework::linear_mixable_helper<lsh_index_storage, lsh_master_table_t>
    mixable_lsh_index_storage;

}  // namespace storage
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_STORAGE_LSH_INDEX_STORAGE_HPP_
