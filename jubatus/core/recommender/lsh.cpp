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

#include "lsh.hpp"

#include <cmath>
#include <algorithm>
#include <string>
#include <utility>
#include <vector>
#include "jubatus/util/lang/function.h"
#include "jubatus/util/lang/bind.h"
#include "../common/exception.hpp"
#include "../common/hash.hpp"
#include "lsh_util.hpp"
#include "../storage/bit_index_storage.hpp"
#include "../unlearner/unlearner_factory.hpp"

using std::pair;
using std::string;
using std::vector;
using jubatus::core::storage::bit_vector;

namespace jubatus {
namespace core {
namespace recommender {

static const uint64_t DEFAULT_HASH_NUM = 64;  // should be in config

lsh::config::config()
    : hash_num(DEFAULT_HASH_NUM) {
}

lsh::lsh(uint64_t hash_num)
    : hash_num_(hash_num) {
  if (!(1 <= hash_num)) {
    throw JUBATUS_EXCEPTION(
        common::invalid_parameter("1 <= hash_num"));
  }
  initialize_model();
}

lsh::lsh(const config& config)
    : hash_num_(config.hash_num) {

  if (!(1 <= config.hash_num)) {
    throw JUBATUS_EXCEPTION(
        common::invalid_parameter("1 <= hash_num"));
  }

  initialize_model();

  if (config.unlearner) {
    if (!config.unlearner_parameter) {
      throw JUBATUS_EXCEPTION(
          common::config_exception() << common::exception::error_message(
              "unlearner is set but unlearner_parameter is not found"));
    }
    unlearner_ = core::unlearner::create_unlearner(*config.unlearner,
            core::common::jsonconfig::config(*config.unlearner_parameter));
    mixable_storage_->get_model()->set_unlearner(unlearner_);
    unlearner_->set_callback(
        util::lang::bind(&lsh::remove_row, this, util::lang::_1));
  } else {
    if (config.unlearner_parameter) {
      throw JUBATUS_EXCEPTION(
          common::config_exception() << common::exception::error_message(
              "unlearner_parameter is set but unlearner is not found"));
    }
  }
}

lsh::lsh()
    : hash_num_(DEFAULT_HASH_NUM) {
  initialize_model();
}

lsh::~lsh() {
}

void lsh::similar_row(
    const common::sfv_t& query,
    vector<pair<string, float> >& ids,
    size_t ret_num) const {
  ids.clear();
  if (ret_num == 0) {
    return;
  }

  bit_vector query_bv;
  calc_lsh_values(query, query_bv);
  mixable_storage_->get_model()->similar_row(query_bv, ids, ret_num);
}

void lsh::neighbor_row(
    const common::sfv_t& query,
    vector<pair<string, float> >& ids,
    size_t ret_num) const {
  similar_row(query, ids, ret_num);
  for (size_t i = 0; i < ids.size(); ++i) {
    ids[i].second = 1 - ids[i].second;
  }
}

void lsh::clear() {
  orig_.clear();
  jubatus::util::data::unordered_map<std::string, std::vector<float> >()
    .swap(column2baseval_);
  mixable_storage_->get_model()->clear();
  if (unlearner_) {
    unlearner_->clear();
  }
}

void lsh::clear_row(const string& id) {
  remove_row(id);
  if (unlearner_) {
    unlearner_->remove(id);
  }
}

void lsh::remove_row(const string& id) {
  orig_.remove_row(id);
  mixable_storage_->get_model()->remove_row(id);
}

void lsh::calc_lsh_values(const common::sfv_t& sfv, bit_vector& bv) const {
  const_cast<lsh*>(this)->generate_column_bases(sfv);

  vector<float> lsh_vals;
  prod_invert_and_vector(column2baseval_, sfv, hash_num_, lsh_vals);
  set_bit_vector(lsh_vals, bv);
}

void lsh::generate_column_bases(const common::sfv_t& sfv) {
  for (size_t i = 0; i < sfv.size(); ++i) {
    generate_column_base(sfv[i].first);
  }
}

void lsh::generate_column_base(const string& column) {
  if (column2baseval_.count(column) != 0) {
    return;
  }
  const uint32_t seed = common::hash_util::calc_string_hash(column);
  generate_random_vector(hash_num_, seed, column2baseval_[column]);
}

void lsh::update_row(const string& id, const sfv_diff_t& diff) {
  if (unlearner_ && !unlearner_->can_touch(id)) {
    throw JUBATUS_EXCEPTION(common::exception::runtime_error(
        "cannot add new row as number of sticky rows reached "
            "the maximum size of unlearner: " + id));
  }
  generate_column_bases(diff);
  orig_.set_row(id, diff);
  common::sfv_t row;
  orig_.get_row(id, row);
  bit_vector bv;
  calc_lsh_values(row, bv);
  mixable_storage_->get_model()->set_row(id, bv);
  if (unlearner_) {
    unlearner_->touch(id);
  }
}

void lsh::get_all_row_ids(std::vector<std::string>& ids) const {
  mixable_storage_->get_model()->get_all_row_ids(ids);
}

string lsh::type() const {
  return string("lsh");
}

framework::mixable* lsh::get_mixable() const {
  return mixable_storage_.get();
}

void lsh::initialize_model() {
  typedef storage::mixable_bit_index_storage::model_ptr model_ptr;
  typedef storage::mixable_bit_index_storage storage_t;
  model_ptr p(new storage::bit_index_storage);
  mixable_storage_.reset(new storage_t(p));
}

void lsh::pack(framework::packer& packer) const {
  packer.pack_array(2);
  orig_.pack(packer);
  mixable_storage_->get_model()->pack(packer);
}

void lsh::unpack(msgpack::object o) {
  if (o.type != msgpack::type::ARRAY || o.via.array.size != 2) {
    throw msgpack::type_error();
  }
  orig_.unpack(o.via.array.ptr[0]);
  mixable_storage_->get_model()->unpack(o.via.array.ptr[1]);
}

}  // namespace recommender
}  // namespace core
}  // namespace jubatus
