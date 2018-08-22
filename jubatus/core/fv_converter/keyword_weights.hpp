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

#ifndef JUBATUS_CORE_FV_CONVERTER_KEYWORD_WEIGHTS_HPP_
#define JUBATUS_CORE_FV_CONVERTER_KEYWORD_WEIGHTS_HPP_

#include <map>
#include <string>
#include <msgpack.hpp>
#include "jubatus/util/data/unordered_map.h"
#include "../common/type.hpp"
#include "counter.hpp"

namespace jubatus {
namespace core {
namespace fv_converter {

class keyword_weights {
 public:
  keyword_weights();

  void update_document_frequency(
      const common::sfv_t& fv,
      bool use_group_frequency);

  inline size_t get_document_frequency(const std::string& key) const {
    return document_frequencies_[key];
  }

  inline uint64_t get_document_count() const {
    return document_count_;
  }

  inline double get_group_frequency(
      const std::string& group_key) const {
    return group_frequencies_[group_key];
  }

  inline double get_group_total_length(
      const std::string& group_key) const {
    return group_total_lengths_[group_key];
  }

  void add_weight(const std::string& key, double weight);

  double get_user_weight(const std::string& key) const;

  void merge(const keyword_weights& w);

  void clear();

  MSGPACK_DEFINE(
      document_count_,
      document_frequencies_,
      group_frequencies_,
      group_total_lengths_,
      weights_);

  std::string to_string() const;

  void get_status(
      std::map<std::string, std::string>& status,
      const std::string& prefix) const;

 private:
  double get_global_weight(const std::string& key) const;

  // Number of documents (datum, feature vector) processed.
  uint64_t document_count_;

  // Number of times each feature vector is observed.
  counter<std::string> document_frequencies_;

  // Number of times each feature group is observed.
  // (see also `key_name_utils.hpp`)
  counter<std::string> group_frequencies_;

  // Total value of each feature group.
  counter<std::string> group_total_lengths_;

  typedef jubatus::util::data::unordered_map<std::string, double> weight_t;
  weight_t weights_;
};

}  // namespace fv_converter
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_FV_CONVERTER_KEYWORD_WEIGHTS_HPP_
