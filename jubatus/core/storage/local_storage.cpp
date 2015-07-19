// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2011-2014 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#include "local_storage.hpp"
#include <cmath>
#include <map>
#include <string>
#include <vector>
#include "jubatus/util/data/intern.h"
#include "jubatus/util/concurrent/lock.h"

using std::string;
using std::vector;
using jubatus::util::concurrent::scoped_lock;


namespace jubatus {
namespace core {
namespace storage {

local_storage::local_storage() {
}

local_storage::~local_storage() {
}

void local_storage::get(const string& feature, feature_val1_t& ret) const {
  scoped_lock lk(mutex_);
  get_nolock(feature, ret);
}

void local_storage::get_nolock(const string& feature,
                               feature_val1_t& ret) const {
  ret.clear();
  id_features3_t::const_iterator cit = tbl_.find(feature);
  if (cit == tbl_.end()) {
    return;
  }
  const id_feature_val3_t& m = cit->second;
  for (id_feature_val3_t::const_iterator it = m.begin(); it != m.end(); ++it) {
    ret.push_back(make_pair(class2id_.get_key(it->first), it->second.v1));
  }
}

void local_storage::get2(const string& feature, feature_val2_t& ret) const {
  scoped_lock lk(mutex_);
  get2_nolock(feature, ret);
}

void local_storage::get2_nolock(const string& feature,
                                feature_val2_t& ret) const {
  ret.clear();
  id_features3_t::const_iterator cit = tbl_.find(feature);
  if (cit == tbl_.end()) {
    return;
  }
  const id_feature_val3_t& m = cit->second;
  for (id_feature_val3_t::const_iterator it = m.begin(); it != m.end(); ++it) {
    ret.push_back(make_pair(class2id_.get_key(it->first),
                            val2_t(it->second.v1, it->second.v2)));
  }
}

void local_storage::get3(const string& feature, feature_val3_t& ret) const {
  scoped_lock lk(mutex_);
  get3_nolock(feature, ret);
}

void local_storage::get3_nolock(const string& feature,
                                feature_val3_t& ret) const {
  ret.clear();
  id_features3_t::const_iterator cit = tbl_.find(feature);
  if (cit == tbl_.end()) {
    return;
  }
  const id_feature_val3_t& m = cit->second;
  for (id_feature_val3_t::const_iterator it = m.begin(); it != m.end(); ++it) {
    ret.push_back(make_pair(class2id_.get_key(it->first), it->second));
  }
}

void local_storage::inp(const common::sfv_t& sfv, map_feature_val1_t& ret)
    const {
  ret.clear();

  scoped_lock lk(mutex_);
  // Use uin64_t map instead of string map as hash function for string is slow
  jubatus::util::data::unordered_map<uint64_t, float> ret_id;
  for (common::sfv_t::const_iterator it = sfv.begin(); it != sfv.end(); ++it) {
    const string& feature = it->first;
    const float val = it->second;
    id_features3_t::const_iterator it2 = tbl_.find(feature);
    if (it2 == tbl_.end()) {
      continue;
    }
    const id_feature_val3_t& m = it2->second;
    for (id_feature_val3_t::const_iterator it3 = m.begin(); it3 != m.end();
        ++it3) {
      ret_id[it3->first] += it3->second.v1 * val;
    }
  }

  std::vector<std::string> labels = class2id_.get_all_id2key();
  for (size_t i = 0; i < labels.size(); ++i) {
    const std::string& label = labels[i];
    uint64_t id = class2id_.get_id_const(label);
    if (id == common::key_manager::NOTFOUND || ret_id.count(id) == 0) {
      ret[label] = 0.0;
    } else {
      ret[label] = ret_id[id];
    }
  }
}

void local_storage::set(
    const string& feature,
    const string& klass,
    const val1_t& w) {
  scoped_lock lk(mutex_);
  set_nolock(feature, klass, w);
  tbl_[feature][class2id_.get_id(klass)].v1 = w;
}
void local_storage::set_nolock(
    const string& feature,
    const string& klass,
    const val1_t& w) {
  tbl_[feature][class2id_.get_id(klass)].v1 = w;
}

void local_storage::set2(
    const string& feature,
    const string& klass,
    const val2_t& w) {
  scoped_lock lk(mutex_);
  set2_nolock(feature, klass, w);
}

void local_storage::set2_nolock(
    const string& feature,
    const string& klass,
    const val2_t& w) {
  val3_t& val3 = tbl_[feature][class2id_.get_id(klass)];
  val3.v1 = w.v1;
  val3.v2 = w.v2;
}

void local_storage::set3(
    const string& feature,
    const string& klass,
    const val3_t& w) {
  scoped_lock lk(mutex_);
  set3_nolock(feature, klass, w);
}

void local_storage::set3_nolock(
    const string& feature,
    const string& klass,
    const val3_t& w) {
  tbl_[feature][class2id_.get_id(klass)] = w;
}

void local_storage::get_status(std::map<string, std::string>& status) const {
  scoped_lock lk(mutex_);
  status["num_features"] =
    jubatus::util::lang::lexical_cast<std::string>(tbl_.size());
  status["num_classes"] =
    jubatus::util::lang::lexical_cast<std::string>(class2id_.size());
}

float feature_fabssum(const id_feature_val3_t& f) {
  float sum = 0.f;
  for (id_feature_val3_t::const_iterator it = f.begin(); it != f.end(); ++it) {
    sum += std::fabs(it->second.v1);
  }
  return sum;
}

void local_storage::bulk_update(
    const common::sfv_t& sfv,
    float step_width,
    const string& inc_class,
    const string& dec_class) {
  scoped_lock lk(mutex_);
  uint64_t inc_id = class2id_.get_id(inc_class);
  typedef common::sfv_t::const_iterator iter_t;
  if (dec_class != "") {
    uint64_t dec_id = class2id_.get_id(dec_class);
    for (iter_t it = sfv.begin(); it != sfv.end(); ++it) {
      float val = it->second * step_width;
      id_feature_val3_t& feature_row = tbl_[it->first];
      feature_row[inc_id].v1 += val;
      feature_row[dec_id].v1 -= val;
    }
  } else {
    for (iter_t it = sfv.begin(); it != sfv.end(); ++it) {
      float val = it->second * step_width;
      id_feature_val3_t& feature_row = tbl_[it->first];
      feature_row[inc_id].v1 += val;
    }
  }
}

void local_storage::update(
    const string& feature,
    const string& inc_class,
    const string& dec_class,
    const val1_t& v) {
  scoped_lock lk(mutex_);
  id_feature_val3_t& feature_row = tbl_[feature];
  feature_row[class2id_.get_id(inc_class)].v1 += v;
  feature_row[class2id_.get_id(dec_class)].v1 -= v;
}

util::concurrent::mutex& local_storage::get_lock() const {
  return mutex_;
}

void local_storage::register_label(const std::string& label) {
  scoped_lock lk(mutex_);
  // get_id method creates an entry when the label doesn't exist
  class2id_.get_id(label);
}

vector<string> local_storage::get_labels() const {
  scoped_lock lk(mutex_);
  return class2id_.get_all_id2key();
}

bool local_storage::set_label(const std::string& label) {
  scoped_lock lk(mutex_);
  return class2id_.set_key(label);
}

bool local_storage::delete_label(const std::string& label) {
  scoped_lock lk(mutex_);
  return delete_label_nolock(label);
}

bool local_storage::delete_label_nolock(const std::string& label) {
  uint64_t delete_id = class2id_.get_id_const(label);
  if (delete_id == common::key_manager::NOTFOUND) {
    return false;
  }
  for (id_features3_t::iterator it = tbl_.begin();
       it != tbl_.end();
       ) {
    const bool deleted = it->second.erase(delete_id);
    if (deleted && it->second.empty()) {
      it = tbl_.erase(it);
    } else {
      ++it;
    }
  }
  class2id_.delete_key(label);
  return true;
}

void local_storage::clear() {
  scoped_lock lk(mutex_);
  // Clear and minimize
  id_features3_t().swap(tbl_);
  common::key_manager().swap(class2id_);
}

void local_storage::pack(framework::packer& packer) const {
  scoped_lock lk(mutex_);
  packer.pack(*this);
}

void local_storage::unpack(msgpack::object o) {
  scoped_lock lk(mutex_);
  o.convert(this);
}

void local_storage::import_model(msgpack::object o) {
  o.convert(this);
}

std::string local_storage::type() const {
  return "local_storage";
}

}  // namespace storage
}  // namespace core
}  // namespace jubatus
