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

#include "minhash.hpp"

#include <algorithm>
#include <cmath>
#include <cfloat>
#include <string>
#include <utility>
#include <vector>
#include "jubatus/util/lang/function.h"
#include "jubatus/util/lang/bind.h"
#include "../common/exception.hpp"
#include "../common/hash.hpp"
#include "../unlearner/unlearner_factory.hpp"

using std::pair;
using std::string;
using std::vector;
using jubatus::util::lang::shared_ptr;
using jubatus::core::storage::bit_vector;

namespace jubatus {
namespace core {
namespace recommender {

const uint64_t minhash::hash_prime = 0xc3a5c85c97cb3127ULL;

minhash::minhash()
    : hash_num_(64) {
  initialize_model();
}

minhash::minhash(const config& config)
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
        util::lang::bind(&minhash::remove_row, this, util::lang::_1));
  } else {
    if (config.unlearner_parameter) {
      throw JUBATUS_EXCEPTION(
          common::config_exception() << common::exception::error_message(
              "unlearner_parameter is set but unlearner is not found"));
    }
  }
}

minhash::~minhash() {
}

void minhash::similar_row(
    const common::sfv_t& query,
    vector<pair<string, double> >& ids,
    size_t ret_num) const {
  ids.clear();
  if (ret_num == 0) {
    return;
  }

  bit_vector query_bv;
  calc_minhash_values(query, query_bv);
  mixable_storage_->get_model()->similar_row(query_bv, ids, ret_num);
}

void minhash::similar_row(
    const string& id,
    vector<pair<string, double> >& ids,
    size_t ret_num) const {
  ids.clear();
  mixable_storage_->get_model()->similar_row(id, ids, ret_num);
}

void minhash::neighbor_row(
    const common::sfv_t& query,
    vector<pair<string, double> >& ids,
    size_t ret_num) const {
  similar_row(query, ids, ret_num);
  for (size_t i = 0; i < ids.size(); ++i) {
    ids[i].second = 1 - ids[i].second;
  }
}

void minhash::neighbor_row(
    const string& id,
    vector<pair<string, double> >& ids,
    size_t ret_num) const {
  similar_row(id, ids, ret_num);
  for (size_t i = 0; i < ids.size(); ++i) {
    ids[i].second = 1 - ids[i].second;
  }
}

void minhash::clear() {
  orig_.clear();
  mixable_storage_->get_model()->clear();
  if (unlearner_) {
    unlearner_->clear();
  }
}

void minhash::clear_row(const string& id) {
  remove_row(id);
  if (unlearner_) {
    unlearner_->remove(id);
  }
}

void minhash::remove_row(const string& id) {
  orig_.remove_row(id);
  mixable_storage_->get_model()->remove_row(id);
}

void minhash::calc_minhash_values(const common::sfv_t& sfv,
                                  bit_vector& bv) const {
  vector<float> min_values_buffer(hash_num_, FLT_MAX);
  vector<uint64_t> hash_buffer(hash_num_);
  for (size_t i = 0; i < sfv.size(); ++i) {
    uint64_t key_hash = common::hash_util::calc_string_hash(sfv[i].first);
    double val = sfv[i].second;
    for (uint64_t j = 0; j < hash_num_; ++j) {
      float hashval = calc_hash(key_hash, j, val);
      if (hashval < min_values_buffer[j]) {
        min_values_buffer[j] = hashval;
        hash_buffer[j] = key_hash;
      }
    }
  }

  bv.resize_and_clear(hash_num_);
  for (size_t i = 0; i < hash_buffer.size(); ++i) {
    if ((hash_buffer[i] & 1LLU) == 1) {
      bv.set_bit(i);
    }
  }
}

void minhash::update_row(const string& id, const sfv_diff_t& diff) {
  if (unlearner_ && !unlearner_->can_touch(id)) {
    throw JUBATUS_EXCEPTION(common::exception::runtime_error(
        "cannot add new row as number of sticky rows reached "
            "the maximum size of unlearner: " + id));
  }

  orig_.set_row(id, diff);
  common::sfv_t row;
  orig_.get_row(id, row);
  bit_vector bv;
  calc_minhash_values(row, bv);
  mixable_storage_->get_model()->set_row(id, bv);
  if (unlearner_) {
    unlearner_->touch(id);
  }
}

void minhash::get_all_row_ids(std::vector<std::string>& ids) const {
  mixable_storage_->get_model()->get_all_row_ids(ids);
}

// original by Hash64 http://burtleburtle.net/bob/hash/evahash.html
void minhash::hash_mix64(uint64_t& a, uint64_t& b, uint64_t& c) {
  a -= b;
  a -= c;
  a ^= (c >> 43);
  b -= c;
  b -= a;
  b ^= (a << 9);
  c -= a;
  c -= b;
  c ^= (b >> 8);
  a -= b;
  a -= c;
  a ^= (c >> 38);
  b -= c;
  b -= a;
  b ^= (a << 23);
  c -= a;
  c -= b;
  c ^= (b >> 5);
  a -= b;
  a -= c;
  a ^= (c >> 35);
  b -= c;
  b -= a;
  b ^= (a << 49);
  c -= a;
  c -= b;
  c ^= (b >> 11);
  a -= b;
  a -= c;
  a ^= (c >> 12);
  b -= c;
  b -= a;
  b ^= (a << 18);
  c -= a;
  c -= b;
  c ^= (b >> 22);
}

float minhash::calc_hash(uint64_t a, uint64_t b, double val) {
  uint64_t c = hash_prime;
  hash_mix64(a, b, c);
  hash_mix64(a, b, c);
  float r = static_cast<float>(a) / static_cast<float>(0xFFFFFFFFFFFFFFFFLLU);
  return -std::log(r) / val;
}

string minhash::type() const {
  return string("minhash");
}

framework::mixable* minhash::get_mixable() const {
  return mixable_storage_.get();
}

void minhash::initialize_model() {
  shared_ptr<storage::bit_index_storage> p(new storage::bit_index_storage);
  mixable_storage_.reset(new storage::mixable_bit_index_storage(p));
}

void minhash::pack(framework::packer& packer) const {
  if (unlearner_) {
    packer.pack_array(3);
    unlearner_->pack(packer);
  } else {
    packer.pack_array(2);
  }

  orig_.pack(packer);
  mixable_storage_->get_model()->pack(packer);
}

void minhash::unpack(msgpack::object o) {
  if (o.type != msgpack::type::ARRAY) {
    throw msgpack::type_error();
  }

  size_t i = 0;

  if (unlearner_) {
    if (o.via.array.size != 3) {
      throw msgpack::type_error();
    }

    unlearner_->unpack(o.via.array.ptr[i]);
    ++i;
  } else if (o.via.array.size != 2) {
    throw msgpack::type_error();
  }

  orig_.unpack(o.via.array.ptr[i]);
  mixable_storage_->get_model()->unpack(o.via.array.ptr[i+1]);
}

}  // namespace recommender
}  // namespace core
}  // namespace jubatus
