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

#include "euclid_lsh.hpp"

#include <cmath>
#include <queue>
#include <utility>
#include <string>
#include <vector>
#include "jubatus/util/data/serialization.h"
#include "jubatus/util/lang/cast.h"
#include "jubatus/util/lang/function.h"
#include "jubatus/util/lang/bind.h"
#include "jubatus/util/math/random.h"
#include "jubatus/util/concurrent/lock.h"
#include "jubatus/util/concurrent/mutex.h"
#include "../common/hash.hpp"
#include "../storage/lsh_util.hpp"
#include "../storage/lsh_vector.hpp"
#include "../storage/lsh_index_storage.hpp"
#include "../unlearner/unlearner_factory.hpp"
#include "../nearest_neighbor/bit_vector_ranking.hpp"

using std::string;
using std::vector;
using std::pair;
using std::ostream;
using std::istream;
using jubatus::util::concurrent::scoped_lock;

namespace jubatus {
namespace core {
namespace recommender {

namespace {

struct greater_second {
  bool operator()(
      const pair<string, double>& l,
      const pair<string, double>& r) const {
    return l.second > r.second;
  }
};

double calc_norm(const common::sfv_t& sfv) {
  double sqnorm = 0;
  for (size_t i = 0; i < sfv.size(); ++i) {
    sqnorm += sfv[i].second * sfv[i].second;
  }
  return std::sqrt(sqnorm);
}

}  // namespace

euclid_lsh::config::config()
    : hash_num(DEFAULT_HASH_NUM),
      table_num(DEFAULT_TABLE_NUM),
      bin_width(DEFAULT_BIN_WIDTH),
      probe_num(DEFAULT_NUM_PROBE),
      seed(DEFAULT_SEED),
      threads(), cache_size() {
}

const uint64_t euclid_lsh::DEFAULT_HASH_NUM = 64;  // should be in config
const uint64_t euclid_lsh::DEFAULT_TABLE_NUM = 4;  // should be in config
const float euclid_lsh::DEFAULT_BIN_WIDTH = 100;  // should be in config
const uint32_t euclid_lsh::DEFAULT_NUM_PROBE = 64;  // should be in config
const uint32_t euclid_lsh::DEFAULT_SEED = 1091;  // should be in config

typedef storage::mixable_lsh_index_storage::model_ptr model_ptr;
typedef storage::lsh_index_storage lsh_index_storage;

euclid_lsh::euclid_lsh()
    : mixable_storage_(new storage::mixable_lsh_index_storage(
        model_ptr(new lsh_index_storage(DEFAULT_HASH_NUM,
                                        DEFAULT_TABLE_NUM,
                                        DEFAULT_SEED)))),
      bin_width_(DEFAULT_BIN_WIDTH),
      num_probe_(DEFAULT_NUM_PROBE), threads_(1), cache_() {
}

euclid_lsh::euclid_lsh(const config& config)
    : mixable_storage_(),
      bin_width_(config.bin_width),
      num_probe_(config.probe_num),
      threads_(nearest_neighbor::read_threads_config(config.threads)) {

  if (!(1 <= config.hash_num)) {
    throw JUBATUS_EXCEPTION(
        common::invalid_parameter("1 <= hash_num"));
  }

  if (!(1 <= config.table_num)) {
    throw JUBATUS_EXCEPTION(
        common::invalid_parameter("1 <= table_num"));
  }

  if (!(0.f < config.bin_width)) {
    throw JUBATUS_EXCEPTION(
        common::invalid_parameter("0.0 < bin_width"));
  }

  if (!(0 <= config.probe_num)) {
    throw JUBATUS_EXCEPTION(
        common::invalid_parameter("0 <= probe_num"));
  }

  if (!(0 <= config.seed)) {
    throw JUBATUS_EXCEPTION(
        common::invalid_parameter("0 <= seed"));
  }

  nearest_neighbor::init_cache_from_config(cache_, config.cache_size);

  typedef storage::mixable_lsh_index_storage mli_storage;
  typedef mli_storage::model_ptr model_ptr;
  typedef storage::lsh_index_storage li_storage;

  model_ptr p(new li_storage(config.hash_num, config.table_num, config.seed));
  mixable_storage_.reset(new mli_storage(p));

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
        util::lang::bind(&euclid_lsh::remove_row, this, util::lang::_1));
  } else {
    if (config.unlearner_parameter) {
      throw JUBATUS_EXCEPTION(
          common::config_exception() << common::exception::error_message(
              "unlearner_parameter is set but unlearner is not found"));
    }
  }
}

euclid_lsh::~euclid_lsh() {
}

void euclid_lsh::neighbor_row(
    const common::sfv_t& query,
    vector<pair<string, double> >& ids,
    size_t ret_num) const {
  similar_row(query, ids, ret_num);
  for (size_t i = 0; i < ids.size(); ++i) {
    ids[i].second = -ids[i].second;
  }
}

void euclid_lsh::neighbor_row(
    const string& id,
    vector<pair<string, double> >& ids,
    size_t ret_num) const {
  similar_row(id, ids, ret_num);
  for (size_t i = 0; i < ids.size(); ++i) {
    ids[i].second = -ids[i].second;
  }
}

void euclid_lsh::similar_row(
    const common::sfv_t& query,
    vector<pair<string, double> >& ids,
    size_t ret_num) const {
  storage::lsh_index_storage& lsh_index = *mixable_storage_->get_model();
  ids.clear();

  const vector<float> hash = calculate_lsh(query);
  const double norm = calc_norm(query);
  lsh_index.similar_row(hash, norm, num_probe_, ret_num, ids);
}

void euclid_lsh::similar_row(
    const string& id,
    vector<pair<string, double> >& ids,
    size_t ret_num) const {
  ids.clear();
  mixable_storage_->get_model()->similar_row(id, ret_num, ids);
}

void euclid_lsh::clear() {
  orig_.clear();
  mixable_storage_->get_model()->clear();

  if (unlearner_) {
    unlearner_->clear();
  }
}

void euclid_lsh::clear_row(const string& id) {
  remove_row(id);
  if (unlearner_) {
    unlearner_->remove(id);
  }
}

void euclid_lsh::remove_row(const string& id) {
  orig_.remove_row(id);
  mixable_storage_->get_model()->remove_row(id);
}

void euclid_lsh::update_row(const string& id, const sfv_diff_t& diff) {
  if (unlearner_ && !unlearner_->can_touch(id)) {
    throw JUBATUS_EXCEPTION(common::exception::runtime_error(
        "cannot add new row as number of sticky rows reached "
            "the maximum size of unlearner: " + id));
  }
  storage::lsh_index_storage& lsh_index = *mixable_storage_->get_model();
  orig_.set_row(id, diff);
  common::sfv_t row;
  orig_.get_row(id, row);

  const vector<float> hash = calculate_lsh(row);
  const double norm = calc_norm(row);
  lsh_index.set_row(id, hash, norm);
  if (unlearner_) {
    unlearner_->touch(id);
  }
}

void euclid_lsh::get_all_row_ids(vector<string>& ids) const {
  mixable_storage_->get_model()->get_all_row_ids(ids);
}

string euclid_lsh::type() const {
  return "euclid_lsh";
}

framework::mixable* euclid_lsh::get_mixable() const {
  return mixable_storage_.get();
}

vector<float> euclid_lsh::calculate_lsh(const common::sfv_t& query) const {
  vector<float> hash = nearest_neighbor::random_projection(
    query, mixable_storage_->get_model()->all_lsh_num(), threads_, cache_);
  for (size_t j = 0; j < hash.size(); ++j) {
    hash[j] /= bin_width_;
  }
  return hash;
}

void euclid_lsh::initialize_model() {
  typedef storage::mixable_lsh_index_storage::model_ptr model_ptr;
  model_ptr p(new storage::lsh_index_storage);
  mixable_storage_.reset(new storage::mixable_lsh_index_storage(p));
}

void euclid_lsh::pack(framework::packer& packer) const {
  if (unlearner_) {
    packer.pack_array(3);
    unlearner_->pack(packer);
  } else {
    packer.pack_array(2);
  }

  orig_.pack(packer);
  mixable_storage_->get_model()->pack(packer);
}

void euclid_lsh::unpack(msgpack::object o) {
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
