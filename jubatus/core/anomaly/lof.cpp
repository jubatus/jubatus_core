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

#include "lof.hpp"

#include <cmath>
#include <limits>
#include <map>
#include <string>
#include <vector>
#include <utility>

#include "jubatus/util/lang/bind.h"

#include "../unlearner/unlearner_base.hpp"

using jubatus::core::anomaly::lof;
using jubatus::util::data::unordered_map;
using jubatus::util::data::unordered_set;
using std::numeric_limits;
using std::string;
using std::vector;

namespace jubatus {
namespace core {
namespace anomaly {

namespace {

double calculate_lof(
    double lrd,
    const unordered_map<string, double>& neighbor_lrd) {
  if (neighbor_lrd.empty()) {
    return lrd == 0 ? 1 : numeric_limits<double>::infinity();
  }

  double sum_neighbor_lrd = 0;
  for (unordered_map<string, double>::const_iterator it = neighbor_lrd.begin();
       it != neighbor_lrd.end(); ++it) {
    sum_neighbor_lrd += it->second;
  }

  if (std::isinf(sum_neighbor_lrd) && std::isinf(lrd)) {
    return 1;
  }

  return sum_neighbor_lrd / (neighbor_lrd.size() * lrd);
}

}  // namespace

lof::lof(
    const lof_storage::config& config,
    jubatus::util::lang::shared_ptr<recommender::recommender_base> nn_engine)
    : mixable_storage_(),
      nn_engine_(nn_engine) {

  if (!(2 <= config.nearest_neighbor_num)) {
    throw JUBATUS_EXCEPTION(
        common::invalid_parameter("2 <= nearest_neighbor_num"));
  }

  if (!(config.nearest_neighbor_num
      <= config.reverse_nearest_neighbor_num)) {
    throw JUBATUS_EXCEPTION(
        common::invalid_parameter(
            "nearest_neighbor_num <= reverse_nearest_neighbor_num"));
  }

  mixable_lof_storage::model_ptr p(new lof_storage(config, nn_engine));
  mixable_storage_.reset(new mixable_lof_storage(p));
}

lof::lof(
    const lof_storage::config& config,
    jubatus::util::lang::shared_ptr<recommender::recommender_base> nn_engine,
    jubatus::util::lang::shared_ptr<unlearner::unlearner_base> unlearner)
    : mixable_storage_(),
      nn_engine_(nn_engine),
      unlearner_(unlearner) {

  if (!(2 <= config.nearest_neighbor_num)) {
    throw JUBATUS_EXCEPTION(
        common::invalid_parameter("2 <= nearest_neighbor_num"));
  }

  if (!(config.nearest_neighbor_num
      <= config.reverse_nearest_neighbor_num)) {
    throw JUBATUS_EXCEPTION(
        common::invalid_parameter(
            "nearest_neighbor_num <= reverse_nearest_neighbor_num"));
  }

  mixable_lof_storage::model_ptr p(new lof_storage(config, nn_engine));
  mixable_storage_.reset(new mixable_lof_storage(p));
  mixable_storage_->get_model()->set_unlearner(unlearner_);
  unlearner_->set_callback(
      util::lang::bind(&lof::remove_row, this, util::lang::_1));
}

lof::~lof() {
}

double lof::calc_anomaly_score(const common::sfv_t& query) const {
  unordered_map<string, double> neighbor_lrd;
  const double lrd = mixable_storage_->get_model()->collect_lrds(
      query, neighbor_lrd);
  return calculate_lof(lrd, neighbor_lrd);
}

double lof::calc_anomaly_score(const string& id) const {
  unordered_map<string, double> neighbor_lrd;
  const double lrd = mixable_storage_->get_model()->collect_lrds(
      id, neighbor_lrd);

  return calculate_lof(lrd, neighbor_lrd);
}

double lof::calc_anomaly_score(
    const string& id,
    const common::sfv_t& query) const {
  unordered_map<string, double> neighbor_lrd;
  const double lrd = mixable_storage_->get_model()->collect_lrds(
      id, query, neighbor_lrd);
  return calculate_lof(lrd, neighbor_lrd);
}

void lof::clear() {
  mixable_storage_->get_model()->clear();
  if (unlearner_) {
    unlearner_->clear();
  }
}

void lof::clear_row(const string& id) {
  remove_row(id);
  if (unlearner_) {
    unlearner_->remove(id);
  }
}

void lof::remove_row(const string& id) {
  mixable_storage_->get_model()->remove_row(id);
}

bool lof::update_row(const string& id, const sfv_diff_t& diff) {
  if (unlearner_ && !unlearner_->can_touch(id)) {
    throw JUBATUS_EXCEPTION(common::exception::runtime_error(
        "cannot add new row as number of sticky rows reached "
        "the maximum size of unlearner: " + id));
  }
  bool updated = mixable_storage_->get_model()->update_row(id, diff);
  if (unlearner_ && updated) {
    unlearner_->touch(id);
  }
  return updated;
}

vector<string> lof::update_bulk(
    const vector<std::pair<string, common::sfv_t> >& diff) {
  vector<std::pair<string, common::sfv_t> > update_entries;
  vector<string> updated_ids;
  unordered_set<string> update_set;

  vector<std::pair<string, common::sfv_t> >::const_iterator it;
  if (unlearner_) {
    for (it = diff.begin(); it < diff.end(); ++it) {
      if (unlearner_->can_touch((*it).first) &&
          mixable_storage_->get_model()->update_row(*it, update_set)) {
        update_entries.push_back(*it);
        unlearner_->touch((*it).first);
        updated_ids.push_back((*it).first);
        update_set.insert((*it).first);
      }
    }
  } else {
    for (it = diff.begin(); it < diff.end(); ++it) {
      if (mixable_storage_->get_model()->update_row(*it, update_set)) {
        updated_ids.push_back((*it).first);
        update_set.insert((*it).first);
      }
    }
  }
  mixable_storage_->get_model()->update_bulk(update_set);
  return updated_ids;
}


bool lof::set_row(const string& id, const common::sfv_t& sfv) {
  remove_row(id);
  return update_row(id, sfv);
}

vector<string> lof::set_bulk(
    const vector<std::pair<string, common::sfv_t> >& diff) {
  throw JUBATUS_EXCEPTION(common::unsupported_method(__func__));
}

void lof::get_all_row_ids(vector<string>& ids) const {
  mixable_storage_->get_model()->get_all_row_ids(ids);
}

void lof::get_status(std::map<string, string>& status) const {
  mixable_storage_->get_model()->get_status(status);

  if (unlearner_) {
    unlearner_->get_status(status);
  }
}

string lof::type() const {
  return "lof";
}

vector<framework::mixable*> lof::get_mixables() const {
  vector<framework::mixable*> mixables;
  mixables.push_back(mixable_storage_.get());
  mixables.push_back(nn_engine_->get_mixable());
  return mixables;
}

void lof::pack(framework::packer& packer) const {
  if (unlearner_) {
    packer.pack_array(3);
    unlearner_->pack(packer);
  } else {
    packer.pack_array(2);
  }

  mixable_storage_->get_model()->pack(packer);
  nn_engine_->pack(packer);
}

void lof::unpack(msgpack::object o) {
  if (o.type != msgpack::type::ARRAY) {
    throw msgpack::type_error();
  }

  size_t i = 0;
  if (unlearner_) {
    if (o.via.array.size != 3) {
      throw msgpack::type_error();
    }

    // clear before load
    mixable_storage_->get_model()->clear();
    nn_engine_->clear();

    unlearner_->unpack(o.via.array.ptr[i]);
    ++i;
  } else if (o.via.array.size != 2) {
    throw msgpack::type_error();
  } else {
    // clear before load
    mixable_storage_->get_model()->clear();
    nn_engine_->clear();
  }

  mixable_storage_->get_model()->unpack(o.via.array.ptr[i]);
  nn_engine_->unpack(o.via.array.ptr[i+1]);
}

}  // namespace anomaly
}  // namespace core
}  // namespace jubatus
