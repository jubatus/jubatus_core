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

#include <cmath>
#include <map>
#include <string>
#include <sstream>
#include <utility>
#include "jubatus/util/lang/cast.h"
#include "../common/type.hpp"
#include "datum_to_fv_converter.hpp"
#include "keyword_weights.hpp"
#include "key_name_utils.hpp"

using std::string;
using std::pair;
using std::stringstream;
using jubatus::core::common::sfv_t;

namespace jubatus {
namespace core {
namespace fv_converter {

namespace {

struct is_zero {
  bool operator()(const pair<string, float>& p) {
    return p.second == 0;
  }
};

}  // namespace

keyword_weights::keyword_weights()
    : document_count_(),
      document_frequencies_(),
      weights_() {
}

void keyword_weights::update_document_frequency(
    const sfv_t& fv,
    bool use_group_frequency) {
  ++document_count_;

  counter<std::string> group_keys;
  for (sfv_t::const_iterator it = fv.begin(); it != fv.end(); ++it) {
    ++document_frequencies_[it->first];
    if (use_group_frequency) {
      const std::string& k = get_group_key_from_key(it->first);
      if (!k.empty()) {
        group_keys[k] = 1;
        group_total_lengths_[k] += it->second;
      }
    }
  }
  if (use_group_frequency) {
    group_frequencies_.add(group_keys);
  }
}

void keyword_weights::add_weight(const string& key, float weight) {
  weights_[key] = weight;
}

float keyword_weights::get_user_weight(const string& key) const {
  weight_t::const_iterator wit = weights_.find(key);
  if (wit != weights_.end()) {
    return wit->second;
  } else {
    return 0;
  }
}

void keyword_weights::merge(const keyword_weights& w) {
  document_count_ += w.document_count_;
  document_frequencies_.add(w.document_frequencies_);
  group_frequencies_.add(w.group_frequencies_);
  group_total_lengths_.add(w.group_total_lengths_);
  weight_t weights(w.weights_);
  weights.insert(weights_.begin(), weights_.end());
  weights_.swap(weights);
}

void keyword_weights::clear() {
  document_count_ = 0;
  document_frequencies_.clear();
  group_frequencies_.clear();
  group_total_lengths_.clear();
  weight_t().swap(weights_);
}

string keyword_weights::to_string() const {
  stringstream ss;
  ss << "document_count: " << document_count_
     << " document_frequencies: " << document_frequencies_
     << " weights: {";
  for (weight_t::const_iterator it = weights_.begin();
       it != weights_.end();
       ++it) {
    ss << it->first << " => " << it->second << std::endl;
  }
  ss << " }";
  return ss.str();
}

void keyword_weights::get_status(
    std::map<std::string, std::string>& status,
    const std::string& prefix) const {
  status[prefix + "_document_count"] =
    jubatus::util::lang::lexical_cast<std::string>(
        document_count_);
  status[prefix + "_num_document_frequencies"] =
    jubatus::util::lang::lexical_cast<std::string>(
        document_frequencies_.size());
  status[prefix + "_num_group_frequencies"] =
    jubatus::util::lang::lexical_cast<std::string>(
        group_frequencies_.size());
  status[prefix + "_num_group_total_lengths"] =
    jubatus::util::lang::lexical_cast<std::string>(
        group_total_lengths_.size());
  status[prefix + "_num_weights"] =
    jubatus::util::lang::lexical_cast<std::string>(weights_.size());
}

}  // namespace fv_converter
}  // namespace core
}  // namespace jubatus
