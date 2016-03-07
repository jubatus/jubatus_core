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

using jubatus::util::concurrent::scoped_lock;

namespace jubatus {
namespace core {
namespace fv_converter {

namespace {

struct is_zero {
  bool operator()(const std::pair<std::string, float>& p) {
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

void weight_manager::update_weight(const common::sfv_t& fv) {
  scoped_lock lk(mutex_);
  diff_weights_.update_document_frequency(fv);
}

void weight_manager::get_weight(common::sfv_t& fv) const {
  scoped_lock lk(mutex_);
  for (common::sfv_t::iterator it = fv.begin(); it != fv.end(); ++it) {
    double global_weight = get_global_weight(it->first);
    it->second = static_cast<float>(it->second * global_weight);
  }
  fv.erase(remove_if(fv.begin(), fv.end(), is_zero()), fv.end());
}

double weight_manager::get_global_weight(const std::string& key) const {
  size_t p = key.find_last_of('/');
  if (p == std::string::npos) {
    return 1.0;
  }
  std::string type = key.substr(p + 1);
  if (type == "bin") {
    return 1.0;
  } else if (type == "idf") {
    double doc_count = get_document_count();
    double doc_freq = get_document_frequency(key);
    return std::log((doc_count + 1) / (doc_freq + 1));
  } else if (type == "weight") {
    p = key.find_last_of('#');
    if (p == std::string::npos) {
      return 0;
    } else {
      return get_user_weight(key.substr(0, p));
    }
  } else {
    return 1;
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
