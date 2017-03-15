// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2016 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#include "inverted_index_regression.hpp"

#include <string>
#include <vector>
#include <map>
#include "../storage/column_table.hpp"
#include "../storage/column_type.hpp"
#include "jubatus/util/concurrent/lock.h"
#include "jubatus/util/concurrent/rwmutex.h"
#include "nearest_neighbor_regression_util.hpp"
#include "jubatus/core/storage/owner.hpp"

using jubatus::util::lang::shared_ptr;
using jubatus::util::concurrent::scoped_lock;
using jubatus::core::storage::owner;
using jubatus::core::storage::column_type;
using jubatus::core::storage::column_table;
using jubatus::core::framework::mixable_versioned_table;

namespace jubatus {
namespace core {
namespace regression {

const size_t DEFAULT_NEIGHBOR_NUM = 10;
const std::string DEFAULT_WEIGHT = "uniform";

inverted_index_regression::config::config()
    : nearest_neighbor_num(DEFAULT_NEIGHBOR_NUM),
      weight(DEFAULT_WEIGHT) {
}

inverted_index_regression::inverted_index_regression(
    const config& conf) :
  mixable_storage_(),
  config_(conf) {
  if (!(conf.nearest_neighbor_num >= 1)) {
    throw JUBATUS_EXCEPTION(common::invalid_parameter(
        "nearest neighbor num >= 1"));
  }
  typedef storage::inverted_index_storage ii_storage;
  typedef storage::mixable_inverted_index_storage mii_storage;
  jubatus::util::lang::shared_ptr<ii_storage> p(new ii_storage);
  mixable_storage_.reset(new mii_storage(p));
  std::vector<column_type> schema;
  values_.reset(new mixable_versioned_table);
  values_->set_model(shared_ptr<column_table> (new column_table));
  schema.push_back(column_type(column_type::float_type));
  values_->get_model()->init(schema);
}

void inverted_index_regression::train(
    const common::sfv_t& fv,
    const float value) {
  std::string id;
  {
    util::concurrent::scoped_lock lk(rand_mutex_);
    id = make_id(rand_);
  }

  {
    util::concurrent::scoped_wlock lk(storage_mutex_);
    storage::inverted_index_storage& inv = *mixable_storage_->get_model();
    for (size_t i = 0; i < fv.size(); ++i) {
      inv.set(fv[i].first, id, fv[i].second);
    }
  }
  values_->get_model()->add(id, owner(""), value);  // lock acquired inside
}


void inverted_index_regression::get_status(
    std::map<std::string, std::string>& status) const {
}

void inverted_index_regression::pack(framework::packer& pk) const {
  pk.pack_array(2);
  {
    util::concurrent::scoped_rlock lk(storage_mutex_);
    mixable_storage_->get_model()->pack(pk);
  }
  values_->get_model()->pack(pk);
}

void inverted_index_regression::unpack(msgpack::object o) {
  if (o.type != msgpack::type::ARRAY || o.via.array.size != 2) {
    throw msgpack::type_error();
  }

  {
    util::concurrent::scoped_wlock lk(storage_mutex_);
    mixable_storage_->get_model()->unpack(o.via.array.ptr[0]);
  }
  values_->get_model()->unpack(o.via.array.ptr[1]);
}

void inverted_index_regression::clear() {
  {
    util::concurrent::scoped_wlock lk(storage_mutex_);
    mixable_storage_->get_model()->clear();
  }
  values_->get_model()->clear();
}

std::string inverted_index_regression::name() const {
  return "inverted_index_regression";
}

std::vector<framework::mixable*> inverted_index_regression::get_mixables() {
  std::vector<framework::mixable*> mixables;
  mixables.push_back(mixable_storage_.get());
  mixables.push_back(values_.get());
  return mixables;
}

}  // namespace regression
}  // namespace core
}  // namespalce jubatus
