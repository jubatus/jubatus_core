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
#include <vector>
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

  void increment_document_count();
  void increment_document_frequency(const std::vector<std::string> keys);
  void increment_key_frequency(const std::string& key, size_t length);

  size_t get_document_frequency(const std::string& key) const {
    return document_frequencies_[key];
  }

  uint64_t get_document_count() const {
    return document_count_;
  }

  size_t get_key_frequency(const std::string& key) const {
    return key_frequencies_[key];
  }

  size_t get_key_total_length(const std::string& key) const {
    return key_total_length_[key];
  }

  void add_weight(const std::string& key, float weight);

  float get_user_weight(const std::string& key) const;

  void merge(const keyword_weights& w);

  void clear();

  MSGPACK_DEFINE(
      document_count_,
      document_frequencies_,
      key_frequencies_,
      key_total_length_,
      weights_);

  std::string to_string() const;

  void get_status(
      std::map<std::string, std::string>& status,
      const std::string& prefix) const;

 private:
  double get_global_weight(const std::string& key) const;

  uint64_t document_count_;
  counter<std::string> document_frequencies_;

  counter<std::string> key_frequencies_;
  counter<std::string> key_total_length_;

  typedef jubatus::util::data::unordered_map<std::string, float> weight_t;
  weight_t weights_;
};

}  // namespace fv_converter
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_FV_CONVERTER_KEYWORD_WEIGHTS_HPP_
