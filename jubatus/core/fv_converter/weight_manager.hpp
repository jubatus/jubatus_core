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

#ifndef JUBATUS_CORE_FV_CONVERTER_WEIGHT_MANAGER_HPP_
#define JUBATUS_CORE_FV_CONVERTER_WEIGHT_MANAGER_HPP_

#include <istream>
#include <ostream>
#include <sstream>
#include <string>
#include <map>
#include <msgpack.hpp>
#include "jubatus/util/data/unordered_map.h"
#include "jubatus/util/concurrent/mutex.h"
#include "jubatus/util/concurrent/lock.h"
#include "../framework/model.hpp"
#include "../common/type.hpp"
#include "../common/version.hpp"
#include "keyword_weights.hpp"
#include "datum_to_fv_converter.hpp"

namespace jubatus {
namespace core {
namespace fv_converter {

const double BM25_K1 = 1.2;
const double BM25_B = 0.75;

struct versioned_weight_diff {
  versioned_weight_diff();
  explicit versioned_weight_diff(const fv_converter::keyword_weights& w);
  versioned_weight_diff(const fv_converter::keyword_weights& w,
                        const storage::version& v);
  versioned_weight_diff& merge(const versioned_weight_diff& target);

  MSGPACK_DEFINE(weights_, version_);

  fv_converter::keyword_weights weights_;
  storage::version version_;
};

class weight_manager : public framework::model {
 public:
  weight_manager();

  void increment_document_count();
  void update_weight(
      const std::string& key,
      const std::string& type_name,
      const splitter_weight_type& weight_type,
      const counter<std::string>& count);
  void add_string_features(
      const std::string& key,
      const std::string& type_name,
      const splitter_weight_type& weight_type,
      const counter<std::string>& count,
      common::sfv_t& ret_fv) const;

  void add_weight(const std::string& key, float weight);

  void get_diff(versioned_weight_diff& diff) const {
    util::concurrent::scoped_lock lk(mutex_);
    diff = versioned_weight_diff(diff_weights_, version_);
  }

  bool put_diff(const versioned_weight_diff& diff) {
    util::concurrent::scoped_lock lk(mutex_);
    if (diff.version_ == version_) {
      master_weights_.merge(diff.weights_);
      diff_weights_.clear();
      version_.increment();
      return true;
    } else {
      return false;
    }
  }

  void mix(
      const versioned_weight_diff& lhs,
      versioned_weight_diff& acc) const {
    if (lhs.version_ == acc.version_) {
      acc.weights_.merge(lhs.weights_);
    } else if (lhs.version_ > acc.version_) {
      acc = lhs;
    }
  }

  void clear() {
    util::concurrent::scoped_lock lk(mutex_);
    diff_weights_.clear();
    master_weights_.clear();
  }

  storage::version get_version() const {
    return version_;
  }

  MSGPACK_DEFINE(version_, diff_weights_, master_weights_);

  void pack(framework::packer& pk) const {
    util::concurrent::scoped_lock lk(mutex_);
    pk.pack(*this);
  }

  void unpack(msgpack::object o) {
    util::concurrent::scoped_lock lk(mutex_);
    o.convert(this);
  }

  std::string to_string() const {
    util::concurrent::scoped_lock lk(mutex_);
    std::stringstream ss;
    ss << "version:" << version_
       << " diff_weights:" << diff_weights_.to_string()
       << " master_weights:" << master_weights_.to_string();
    return ss.str();
  }

  void get_status(std::map<std::string, std::string>& status) const;

 private:
  uint64_t get_document_count() const {
    return diff_weights_.get_document_count() +
        master_weights_.get_document_count();
  }

  size_t get_document_frequency(const std::string& key) const {
    return diff_weights_.get_document_frequency(key) +
        master_weights_.get_document_frequency(key);
  }

  double get_average_key_length(
       const std::string& key,
      const std::string& type_name) const {
    const std::string& weight_name = make_weight_name(key, "", type_name);

    size_t frequency = diff_weights_.get_key_frequency(weight_name) +
                       master_weights_.get_key_frequency(weight_name);
    if (frequency == 0) {
      return 0;
    }

    size_t total_length = diff_weights_.get_key_total_length(weight_name) +
                          master_weights_.get_key_total_length(weight_name);

    return lexical_cast<double>(total_length) / frequency;
  }

  double get_user_weight(const std::string& key) const {
    return diff_weights_.get_user_weight(key) +
        master_weights_.get_user_weight(key);
  }

  double get_sample_weight(
      frequency_weight_type type,
      const std::string& key,
      const std::string& type_name,
      double tf,
      size_t length) const;

  double get_global_weight(
      term_weight_type type,
      const std::string& fv_name,
      const std::string& weight_name) const;

  mutable util::concurrent::mutex mutex_;
  storage::version version_;
  keyword_weights diff_weights_;
  keyword_weights master_weights_;
};

}  // namespace fv_converter
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_FV_CONVERTER_WEIGHT_MANAGER_HPP_
