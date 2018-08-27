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
#include <map>
#include "../common/type.hpp"
#include "datum_to_fv_converter.hpp"
#include "jubatus/util/concurrent/lock.h"
#include "key_name_utils.hpp"

using jubatus::util::concurrent::scoped_lock;

namespace jubatus {
namespace core {
namespace fv_converter {

namespace {

struct is_zero {
  bool operator()(const std::pair<std::string, double>& p) {
    return p.second == 0;
  }
};

}  // namespace

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

void weight_manager::update_weight(
    const common::sfv_t& fv,
    bool contains_idf,
    bool contains_bm25) {
  scoped_lock lk(mutex_);
  if (contains_idf || contains_bm25) {
    diff_weights_.update_document_frequency(fv, contains_bm25);
  }
}

void weight_manager::get_weight(common::sfv_t& fv) const {
  scoped_lock lk(mutex_);
  counter<std::string> group_lengths;
  for (common::sfv_t::iterator it = fv.begin(); it != fv.end(); ++it) {
    double weight = it->second;
    const std::string& global_weight_type =
        get_global_weight_type_from_key(it->first);

    if (global_weight_type.empty()) {
      // Non-string features.
    } else if (global_weight_type == "bin") {
      // No weighting.
    } else if (global_weight_type == "idf") {
      // IDF weighting.
      weight = get_global_weight_idf(it->first, 0.0, weight);
    } else if (global_weight_type == "idf1") {
      // IDF + 1.0 weighting.
      weight = get_global_weight_idf(it->first, 1.0, weight);
    } else if (global_weight_type == "bm25") {
      // BM25 weighting.

      // Initialize group lengths for the current feature vector, if not yet.
      if (group_lengths.size() == 0) {
        for (common::sfv_t::iterator it2 = fv.begin(); it2 != fv.end(); ++it2) {
          const std::string& k = get_group_key_from_key(it2->first);
          if (!k.empty()) {
            group_lengths[k] += it2->second;
          }
        }
      }
      weight = get_global_weight_bm25(
          it->first, weight, group_lengths);
    } else if (global_weight_type == "weight") {
      weight = get_global_weight_user(it->first, weight);
    } else {
      // Unknown weighting; ignored.
    }

    it->second = weight;
  }
  fv.erase(remove_if(fv.begin(), fv.end(), is_zero()), fv.end());
}

double weight_manager::get_global_weight_idf(
    const std::string& key,
    double inflate_idf,
    double sample_weight) const {
  double doc_count = get_document_count();
  double doc_freq = get_document_frequency(key);
  return (std::log((doc_count + 1) / (doc_freq + 1)) + inflate_idf)
         * sample_weight;
}

double weight_manager::get_global_weight_bm25(
    const std::string& key,
    double sample_weight,
    const counter<std::string>& group_lengths) const {
  const std::string& group_key = get_group_key_from_key(key);

  double doc_count = get_document_count();
  double doc_freq = get_document_frequency(key);
  double group_length = group_lengths[group_key];
  double avg_group_length = get_average_group_length(group_key);

  return std::log((doc_count - doc_freq + 0.5) / (doc_freq + 0.5)) *
      ((sample_weight * (BM25_k1 + 1)) /
      (sample_weight + BM25_k1 * (1 - BM25_b + BM25_b *
      (static_cast<double>(group_length) / avg_group_length))));
}

double weight_manager::get_global_weight_user(
    const std::string& key,
    double sample_weight) const {
  size_t p = key.find_last_of('#');
  if (p == std::string::npos) {
    return 0;
  } else {
    return get_user_weight(key.substr(0, p)) * sample_weight;
  }
}

void weight_manager::add_weight(const std::string& key, double weight) {
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
