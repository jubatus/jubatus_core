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

#include "local_storage_mixture.hpp"
#include <cmath>
#include <map>
#include <string>
#include <vector>
#include "jubatus/util/data/intern.h"

using std::string;

namespace jubatus {
namespace core {
namespace storage {

namespace {

void increase(val3_t& a, const val3_t& b) {
  a.v1 += b.v1;
  a.v2 += b.v2;
  a.v3 += b.v3;
}

void delete_label_from_weight(uint64_t delete_id, id_features3_t& tbl) {
  for (id_features3_t::iterator it = tbl.begin(); it != tbl.end(); ) {
    it->second.erase(delete_id);
    if (it->second.empty()) {
      it = tbl.erase(it);
    } else {
      ++it;
    }
  }
}

}  // namespace

local_storage_mixture::local_storage_mixture() {
}

local_storage_mixture::~local_storage_mixture() {
}

bool local_storage_mixture::get_internal(
    const string& feature,
    id_feature_val3_t& ret) const {
  ret.clear();
  id_features3_t::const_iterator it = tbl_.find(feature);

  bool found = false;
  if (it != tbl_.end()) {
    ret = it->second;
    found = true;
  }

  id_features3_t::const_iterator it_diff = tbl_diff_.find(feature);
  if (it_diff != tbl_diff_.end()) {
    found = true;
    for (id_feature_val3_t::const_iterator it2 = it_diff->second.begin();
        it2 != it_diff->second.end(); ++it2) {
      val3_t& val3 = ret[it2->first];  // may create
      increase(val3, it2->second);
    }
  }
  return found;
}

void local_storage_mixture::get(
    const std::string& feature,
    feature_val1_t& ret) const {
  util::concurrent::scoped_rlock lk(mutex_);
  get_nolock(feature, ret);
}
void local_storage_mixture::get_nolock(
    const std::string& feature,
    feature_val1_t& ret) const {
  ret.clear();
  id_feature_val3_t m3;
  get_internal(feature, m3);
  for (id_feature_val3_t::const_iterator it = m3.begin(); it != m3.end();
      ++it) {
    ret.push_back(make_pair(class2id_.get_key(it->first), it->second.v1));
  }
}

void local_storage_mixture::get2(
    const std::string& feature,
    feature_val2_t& ret) const {
  util::concurrent::scoped_rlock lk(mutex_);
  get2_nolock(feature, ret);
}
void local_storage_mixture::get2_nolock(
    const std::string& feature,
    feature_val2_t& ret) const {
  ret.clear();
  id_feature_val3_t m3;
  get_internal(feature, m3);
  for (id_feature_val3_t::const_iterator it = m3.begin(); it != m3.end();
      ++it) {
    ret.push_back(
        make_pair(class2id_.get_key(it->first),
                  val2_t(it->second.v1, it->second.v2)));
  }
}

void local_storage_mixture::get3(
    const std::string& feature,
    feature_val3_t& ret) const {
  util::concurrent::scoped_rlock lk(mutex_);
  get3_nolock(feature, ret);
}
void local_storage_mixture::get3_nolock(
    const std::string& feature,
    feature_val3_t& ret) const {
  ret.clear();
  id_feature_val3_t m3;
  get_internal(feature, m3);
  for (id_feature_val3_t::const_iterator it = m3.begin(); it != m3.end();
      ++it) {
    ret.push_back(make_pair(class2id_.get_key(it->first), it->second));
  }
}

void local_storage_mixture::inp(const common::sfv_t& sfv,
                                map_feature_val1_t& ret) const {
  ret.clear();

  util::concurrent::scoped_rlock lk(mutex_);
  // Use uin64_t map instead of string map as hash function for string is slow
  jubatus::util::data::unordered_map<uint64_t, float> ret_id;
  for (common::sfv_t::const_iterator it = sfv.begin(); it != sfv.end(); ++it) {
    const string& feature = it->first;
    const float val = it->second;
    id_feature_val3_t m;
    get_internal(feature, m);
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

void local_storage_mixture::set(
    const string& feature,
    const string& klass,
    const val1_t& w) {
  util::concurrent::scoped_wlock lk(mutex_);
  set_nolock(feature, klass, w);
}
void local_storage_mixture::set_nolock(
    const string& feature,
    const string& klass,
    const val1_t& w) {
  uint64_t class_id = class2id_.get_id(klass);
  float w_in_table = tbl_[feature][class_id].v1;
  tbl_diff_[feature][class_id].v1 = w - w_in_table;
}

void local_storage_mixture::set2(
    const string& feature,
    const string& klass,
    const val2_t& w) {
  util::concurrent::scoped_wlock lk(mutex_);
  set2_nolock(feature, klass, w);
}
void local_storage_mixture::set2_nolock(
    const string& feature,
    const string& klass,
    const val2_t& w) {
  uint64_t class_id = class2id_.get_id(klass);
  float w1_in_table = tbl_[feature][class_id].v1;
  float w2_in_table = tbl_[feature][class_id].v2;

  val3_t& triple = tbl_diff_[feature][class_id];
  triple.v1 = w.v1 - w1_in_table;
  triple.v2 = w.v2 - w2_in_table;
}

void local_storage_mixture::set3(
    const string& feature,
    const string& klass,
    const val3_t& w) {
  util::concurrent::scoped_wlock lk(mutex_);
  set3_nolock(feature, klass, w);
}
void local_storage_mixture::set3_nolock(
    const string& feature,
    const string& klass,
    const val3_t& w) {
  uint64_t class_id = class2id_.get_id(klass);
  val3_t v = tbl_[feature][class_id];
  tbl_diff_[feature][class_id] = w - v;
}

void local_storage_mixture::get_status(
    std::map<std::string, std::string>& status) const {
  util::concurrent::scoped_rlock lk(mutex_);
  status["num_features"] =
    jubatus::util::lang::lexical_cast<std::string>(tbl_.size());
  status["num_classes"] = jubatus::util::lang::lexical_cast<std::string>(
      class2id_.size());
  status["diff_size"] =
    jubatus::util::lang::lexical_cast<std::string>(tbl_diff_.size());
  status["model_version"] = jubatus::util::lang::lexical_cast<std::string>(
      model_version_.get_number());
}

void local_storage_mixture::update(
    const string& feature,
    const string& inc_class,
    const string& dec_class,
    const val1_t& v) {
  util::concurrent::scoped_wlock lk(mutex_);
  id_feature_val3_t& feature_row = tbl_diff_[feature];
  feature_row[class2id_.get_id(inc_class)].v1 += v;
  feature_row[class2id_.get_id(dec_class)].v1 -= v;
}

void local_storage_mixture::bulk_update(
    const common::sfv_t& sfv,
    float step_width,
    const string& inc_class,
    const string& dec_class) {
  util::concurrent::scoped_wlock lk(mutex_);
  uint64_t inc_id = class2id_.get_id(inc_class);
  typedef common::sfv_t::const_iterator iter_t;
  if (dec_class != "") {
    uint64_t dec_id = class2id_.get_id(dec_class);
    for (iter_t it = sfv.begin(); it != sfv.end(); ++it) {
      float val = it->second * step_width;
      id_feature_val3_t& feature_row = tbl_diff_[it->first];
      feature_row[inc_id].v1 += val;
      feature_row[dec_id].v1 -= val;
    }
  } else {
    for (iter_t it = sfv.begin(); it != sfv.end(); ++it) {
      float val = it->second * step_width;
      id_feature_val3_t& feature_row = tbl_diff_[it->first];
      feature_row[inc_id].v1 += val;
    }
  }
}

void local_storage_mixture::get_diff(diff_t& ret) const {
  util::concurrent::scoped_rlock lk(mutex_);
  ret.diff.clear();
  for (jubatus::util::data::unordered_map<string, id_feature_val3_t>::
      const_iterator it = tbl_diff_.begin(); it != tbl_diff_.end(); ++it) {
    id_feature_val3_t::const_iterator it2 = it->second.begin();
    feature_val3_t fv3;
    for (; it2 != it->second.end(); ++it2) {
      fv3.push_back(make_pair(class2id_.get_key(it2->first), it2->second));
    }
    ret.diff.push_back(make_pair(it->first, fv3));
  }
  ret.expect_version = model_version_;
}

bool local_storage_mixture::set_average_and_clear_diff(
    const diff_t& average) {
  util::concurrent::scoped_wlock lk(mutex_);
  if (average.expect_version == model_version_) {
    for (features3_t::const_iterator it = average.diff.begin();
         it != average.diff.end();
         ++it) {
      const feature_val3_t& avg = it->second;
      id_feature_val3_t& orig = tbl_[it->first];
      for (feature_val3_t::const_iterator it2 = avg.begin(); it2 != avg.end();
           ++it2) {
        val3_t& triple = orig[class2id_.get_id(it2->first)];  // may create
        increase(triple, it2->second);
      }
    }
    model_version_.increment();
    tbl_diff_.clear();
    return true;
  } else {
    return false;
  }
}

void local_storage_mixture::register_label(const std::string& label) {
  util::concurrent::scoped_wlock lk(mutex_);
  // get_id method creates an entry when the label doesn't exist
  class2id_.get_id(label);
}

bool local_storage_mixture::delete_label(const std::string& label) {
  util::concurrent::scoped_wlock lk(mutex_);
  return delete_label_nolock(label);
}

bool local_storage_mixture::delete_label_nolock(const std::string& label) {
  uint64_t delete_id = class2id_.get_id_const(label);
  if (delete_id == common::key_manager::NOTFOUND) {
    return false;
  }
  delete_label_from_weight(delete_id, tbl_);
  delete_label_from_weight(delete_id, tbl_diff_);
  class2id_.delete_key(label);
  return true;
}

void local_storage_mixture::clear() {
  util::concurrent::scoped_wlock lk(mutex_);
  // Clear and minimize
  id_features3_t().swap(tbl_);
  common::key_manager().swap(class2id_);
  id_features3_t().swap(tbl_diff_);
}

std::vector<std::string> local_storage_mixture::get_labels() const {
  util::concurrent::scoped_rlock lk(mutex_);
  return class2id_.get_all_id2key();
}

bool local_storage_mixture::set_label(const std::string& label) {
  util::concurrent::scoped_wlock lk(mutex_);
  return class2id_.set_key(label);
}

void local_storage_mixture::pack(framework::packer& packer) const {
  util::concurrent::scoped_rlock lk(mutex_);
  packer.pack(*this);
}

void local_storage_mixture::unpack(msgpack::object o) {
  util::concurrent::scoped_wlock lk(mutex_);
  o.convert(this);
}

std::string local_storage_mixture::type() const {
  return "local_storage_mixture";
}

}  // namespace storage
}  // namespace core
}  // namespace jubatus
