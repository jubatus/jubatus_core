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

#include "nearest_neighbor_regression.hpp"

#include <cfloat>
#include <string>
#include <vector>
#include <map>
#include <utility>
#include "../framework/mixable_versioned_table.hpp"
#include "../common/unordered_map.hpp"
#include "../storage/column_table.hpp"
#include "../storage/column_type.hpp"
#include "jubatus/util/concurrent/lock.h"
#include "jubatus/util/lang/bind.h"
#include "jubatus/util/lang/function.h"
#include "jubatus/core/storage/owner.hpp"
#include "nearest_neighbor_regression_util.hpp"

using jubatus::util::lang::shared_ptr;
using jubatus::util::concurrent::scoped_lock;
using jubatus::core::storage::owner;
using jubatus::core::storage::column_type;
using jubatus::core::storage::column_table;
using jubatus::core::framework::mixable_versioned_table;

namespace jubatus {
namespace core {
namespace regression {

class nearest_neighbor_regression::unlearning_callback {
 public:
  explicit unlearning_callback(nearest_neighbor_regression* regression)
      : regression_(regression) {
  }

  void operator()(const std::string& id) {
    regression_->unlearn_id(id);
  }

 private:
  nearest_neighbor_regression* regression_;
};

nearest_neighbor_regression::nearest_neighbor_regression(
    shared_ptr<nearest_neighbor::nearest_neighbor_base> engine,
    const config& conf)
  : nearest_neighbor_engine_(engine), config_(conf) {
  if (!(conf.nearest_neighbor_num >= 1)) {
    throw JUBATUS_EXCEPTION(common::invalid_parameter(
        "nearest_neighbor_num should >= 1"));
  }
  std::vector<column_type> schema;
  values_.reset(new mixable_versioned_table);
  values_->set_model(shared_ptr<column_table> (new column_table));
  schema.push_back(column_type(column_type::float_type));
  values_->get_model()->init(schema);
}

void nearest_neighbor_regression::train(
    const common::sfv_t& fv, const float value) {
  std::string id;
  {
    util::concurrent::scoped_lock lk(rand_mutex_);
    id = make_id(rand_);
  }
  // unlearner is not called in set_row
  if (unlearner_) {
    util::concurrent::scoped_lock unlearner_lk(unlearner_mutex_);

    // acquire the lock outside of touch() function
    shared_ptr<storage::column_table> table =
        nearest_neighbor_engine_->get_table();
    util::concurrent::scoped_wlock lk(table->get_mutex());

    if (!unlearner_->touch(id)) {
      throw JUBATUS_EXCEPTION(common::exception::runtime_error(
          "cannot add new ID as number of sticky IDs reached "
          "the maximum size of unlearner: " + id));
    }
  }
  nearest_neighbor_engine_->set_row(id, fv);  // lock acquired inside
  values_->get_model()->add(id, owner(""), value);
}

void nearest_neighbor_regression::set_unlearner(
    shared_ptr<unlearner::unlearner_base> unlearner) {

  unlearner->set_callback(unlearning_callback(this));
  unlearner_ = unlearner;
  // Support unlearning in MIX
  dynamic_cast<framework::mixable_versioned_table*>
      (nearest_neighbor_engine_->get_mixable())->set_unlearner(unlearner_);
}

float nearest_neighbor_regression::estimate(
    const common::sfv_t& fv) const {
  std::vector<std::pair<std::string, float> > ids;
  // lock acquired inside
  nearest_neighbor_engine_->neighbor_row(fv, ids, config_.nearest_neighbor_num);

  if (ids.size() > 0) {
    float sum = 0.0;
    if (config_.weight && *config_.weight == "distance") {
      float sum_w = 0.0;
      for (std::vector<std::pair<std::string, float> >::const_iterator
               it = ids.begin(); it != ids.end(); ++it) {
        const std::pair<bool, uint64_t> index =
            values_->get_model()->exact_match(it->first);
        float t = values_->get_model()->get_float_column(0)[index.second];
        float d = it->second;
        if (d == 0.0) {
          // In case distance equals zero, returns the vector's target value.
          return t;
        } else {
          float w = 1.0 / d;
          sum += w * t;
          sum_w += w;
        }
      }
      return sum / sum_w;
    } else {
      for (std::vector<std::pair<std::string, float> >:: const_iterator
               it = ids.begin(); it != ids.end(); ++it) {
        const std::pair<bool, uint64_t> index =
            values_->get_model()->exact_match(it->first);
        sum += values_->get_model()->get_float_column(0)[index.second];
    }
      return sum / ids.size();
    }
  } else {
    return 0;
  }
}

void nearest_neighbor_regression::clear() {
  nearest_neighbor_engine_->clear();  // lock acquired inside
  values_->get_model()->clear();
  if (unlearner_) {
    unlearner_->clear();
  }
}

std::string nearest_neighbor_regression::name() const {
  return "nearest_neighbor_regression:" + nearest_neighbor_engine_->type();
}

void nearest_neighbor_regression::get_status(
    std::map<std::string, std::string>& status) const {
  // unimplemented
}

void nearest_neighbor_regression::pack(framework::packer& pk) const {
  pk.pack_array(2);
  nearest_neighbor_engine_->pack(pk);
  values_->get_model()->pack(pk);
}

void nearest_neighbor_regression::unpack(msgpack::object o) {
  if (o.type != msgpack::type::ARRAY || o.via.array.size != 2) {
    throw msgpack::type_error();
  }
  nearest_neighbor_engine_->unpack(o.via.array.ptr[0]);
  values_->get_model()->unpack(o.via.array.ptr[1]);
}

std::vector<framework::mixable*> nearest_neighbor_regression::get_mixables() {
  std::vector<framework::mixable*> mixables;
  mixables.push_back(nearest_neighbor_engine_->get_mixable());
  mixables.push_back(values_.get());
  return mixables;
}

void nearest_neighbor_regression::unlearn_id(const std::string& id) {
  // This method must be called via touch() function.
  // touch() must be done with holding lock
  // so this function must not get lock
  nearest_neighbor_engine_->get_table()->delete_row_nolock(id);
  values_->get_model()->delete_row_nolock(id);
}

}  // namespace regression
}  // namespace core
}  // namespace jubatus

