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

#include "weight_manager.hpp"

#include <cmath>
#include <string>
#include <utility>
#include <vector>
#include <map>

#include "../common/type.hpp"
#include "datum_to_fv_converter.hpp"
#include "jubatus/util/concurrent/lock.h"

using jubatus::util::concurrent::scoped_lock;

namespace jubatus {
namespace core {
namespace fv_converter {

versioned_weight_diff::versioned_weight_diff() {
}

versioned_weight_diff::versioned_weight_diff(const keyword_weights& w)
  : weights_(w) {
}

versioned_weight_diff::versioned_weight_diff(
    const keyword_weights& w,
    const storage::version& v)
  : weights_(w), version_(v) {
}

versioned_weight_diff&
versioned_weight_diff::merge(const versioned_weight_diff& target) {
  if (version_ == target.version_) {
    weights_.merge(target.weights_);
  } else if (version_ < target.version_) {
    weights_ = target.weights_;
    version_ = target.version_;
  }
  return *this;
}

weight_manager::weight_manager()
    : diff_weights_(),
      master_weights_() {
}

/**
 * Increments the document count.
 * Must be called each time Datum is processed.
 */
void weight_manager::increment_document_count() {
  scoped_lock lk(mutex_);  // to modify weights
  diff_weights_.increment_document_count();
}

/**
 * Updates the weight.
 */
void weight_manager::update_weight(
    const std::string& key,
    const std::string& type_name,
    const splitter_weight_type& weight_type,
    const counter<std::string>& count) {
  std::vector<std::string> keys;
  keys.reserve(count.size());
  for (counter<std::string>::const_iterator it = count.begin();
       it != count.end(); ++it) {
    keys.push_back(make_string_feature_name(
        key,
        it->first,
        type_name,
        weight_type.freq_weight_type_,
        weight_type.term_weight_type_));
  }

  scoped_lock lk(mutex_);  // to modify weights
  diff_weights_.increment_document_frequency(keys);
}

/**
 * Returns sample-weighted term frequency.
 */
double weight_manager::get_sample_weight(
    frequency_weight_type type,
    double tf) const {
  switch (type) {
    case FREQ_BINARY:
      return 1.0;
    case TERM_FREQUENCY:
      return tf;
    case LOG_TERM_FREQUENCY:
      return std::log(1. + tf);
    default:
      return 1.0;
  }
}

/**
 * Returns global-weighted value.
 * This function is thread-unsafe.  Mutex lock must be taken outside.
 */
double weight_manager::get_global_weight(
    term_weight_type type,
    const std::string& fv_name,
    const std::string& weight_name) const {
  double doc_count = get_document_count();
  double doc_freq = get_document_frequency(fv_name);

  switch (type) {
    case TERM_BINARY:
      return 1.0;
    case IDF:
      return std::log((doc_count + 1) / (doc_freq + 1));
    case WITH_WEIGHT_FILE:
      return get_user_weight(weight_name);
    default:
      return 1.0;
  }
}

/**
 * Add string features to the Sparse Feature Vector.
 */
void weight_manager::add_string_features(
    const std::string& key,
    const std::string& type_name,
    const splitter_weight_type& weight_type,
    const counter<std::string>& count,
    common::sfv_t& ret_fv) const {
  scoped_lock lk(mutex_);

  for (counter<std::string>::const_iterator it = count.begin();
       it != count.end(); ++it) {
    const std::string& value = it->first;
    const double tf = it->second;
    std::string f = make_string_feature_name(
        key,
        value,
        type_name,
        weight_type.freq_weight_type_,
        weight_type.term_weight_type_);
    std::string weight_name = make_weight_name(key, value, type_name);

    double sample_weight = get_sample_weight(
        weight_type.freq_weight_type_,
        tf);

    double global_weight = get_global_weight(
        weight_type.term_weight_type_,
        f,
        weight_name);

    float v = static_cast<float>(sample_weight * global_weight);
    if (v != 0.0) {
      ret_fv.push_back(std::make_pair(f, v));
    }
  }
}

void weight_manager::add_weight(const std::string& key, float weight) {
  scoped_lock lk(mutex_);
  diff_weights_.add_weight(key, weight);
}

void weight_manager::get_status(
    std::map<std::string, std::string>& status) const {
  scoped_lock lk(mutex_);
  status["weight_manager_version"] =
    jubatus::util::lang::lexical_cast<std::string>(
        version_.get_number());
  diff_weights_.get_status(status, "diff");
  master_weights_.get_status(status, "master");
}

}  // namespace fv_converter
}  // namespace core
}  // namespace jubatus
