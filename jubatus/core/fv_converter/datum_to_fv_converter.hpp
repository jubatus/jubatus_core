// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2011 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#ifndef JUBATUS_CORE_FV_CONVERTER_DATUM_TO_FV_CONVERTER_HPP_
#define JUBATUS_CORE_FV_CONVERTER_DATUM_TO_FV_CONVERTER_HPP_

#include <string>
#include <utility>
#include <vector>
#include "jubatus/util/data/unordered_map.h"
#include "jubatus/util/lang/shared_ptr.h"
#include "jubatus/util/lang/scoped_ptr.h"
#include "jubatus/util/lang/cast.h"
#include "../common/type.hpp"
#include "../framework/mixable.hpp"
#include "exception.hpp"

using jubatus::util::lang::lexical_cast;

namespace jubatus {
namespace core {
namespace fv_converter {

/**
 * sample_weight
 */
enum frequency_weight_type {
  FREQ_BINARY,
  TERM_FREQUENCY,
  LOG_TERM_FREQUENCY
};

/**
 * global_weight
 */
enum term_weight_type {
  TERM_BINARY,
  IDF,
  WITH_WEIGHT_FILE
};

/**
 * Converts sample_weight name to enum.
 */
frequency_weight_type get_frequency_weight_type(const std::string& name);

/**
 * Converts sample_weight enum to name.
 */
std::string get_frequency_weight_name(frequency_weight_type type);

/**
 * Converts global_weight name to enum.
 */
term_weight_type get_term_weight_type(const std::string& name);

/**
 * Converts global_weight enum to name.
 */
std::string get_term_weight_name(term_weight_type type);

/**
 * A pair of sample_weight and global_weight configuration.
 */
struct splitter_weight_type {
  frequency_weight_type freq_weight_type_;
  term_weight_type term_weight_type_;

  splitter_weight_type(
      frequency_weight_type freq_weight_type,
      term_weight_type term_weight_type)
      : freq_weight_type_(freq_weight_type),
        term_weight_type_(term_weight_type) {
  }
};

/**
 * Creates a key name of Sparse Feature Vector for string features.
 */
std::string make_string_feature_name(
    const std::string& key,
    const std::string& value,
    const std::string& type,
    frequency_weight_type sample_weight,
    term_weight_type global_weight);

/**
 * Creates a key name of Sparse Feature Vector for numeric features.
 */
std::string make_num_feature_name(
    const std::string& key,
    const std::string& type);

/**
 * Creates a key name of Sparse Feature Vector for binary features.
 */
std::string make_binary_feature_name(
    const std::string& key,
    const std::string& type);

/**
 * Creates a key name of user defined weight.
 */
std::string make_weight_name(
    const std::string& key,
    const std::string& value,
    const std::string& type);

struct datum;
class datum_to_fv_converter_impl;
class key_matcher;
class num_feature;
class binary_feature;
class combination_feature;
class string_filter;
class num_filter;
class string_feature;
class weight_manager;

class datum_to_fv_converter {
 public:
  datum_to_fv_converter();

  ~datum_to_fv_converter();

  void convert(const datum& datum, common::sfv_t& ret_fv) const;

  void convert_and_update_weight(const datum& datum, common::sfv_t& ret_fv);

  void clear_rules();

  void register_string_filter(
      jubatus::util::lang::shared_ptr<key_matcher> matcher,
      jubatus::util::lang::shared_ptr<string_filter> filter,
      const std::string& suffix);

  void register_num_filter(
      jubatus::util::lang::shared_ptr<key_matcher> matcher,
      jubatus::util::lang::shared_ptr<num_filter> filter,
      const std::string& suffix);

  void register_string_rule(
      const std::string& name,
      jubatus::util::lang::shared_ptr<key_matcher> matcher,
      jubatus::util::lang::shared_ptr<string_feature> splitter,
      const std::vector<splitter_weight_type>& weights);

  void register_num_rule(
      const std::string& name,
      jubatus::util::lang::shared_ptr<key_matcher> matcher,
      jubatus::util::lang::shared_ptr<num_feature> feature_func);

  void register_binary_rule(
      const std::string& name,
      jubatus::util::lang::shared_ptr<key_matcher> matcher,
      jubatus::util::lang::shared_ptr<binary_feature> feature_func);

  void register_combination_rule(
      const std::string& name,
      jubatus::util::lang::shared_ptr<key_matcher> matcher_left,
      jubatus::util::lang::shared_ptr<key_matcher> matcher_right,
      jubatus::util::lang::shared_ptr<combination_feature> feature_func);

  void add_weight(const std::string& key, float weight);

  void revert_feature(
      const std::string& feature,
      std::pair<std::string, std::string>& expect) const;

  void set_hash_max_size(uint64_t hash_max_size);

  void set_weight_manager(jubatus::util::lang::shared_ptr<weight_manager> wm);
  void clear_weights();

 private:
  jubatus::util::lang::scoped_ptr<datum_to_fv_converter_impl> pimpl_;
};

}  // namespace fv_converter
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_FV_CONVERTER_DATUM_TO_FV_CONVERTER_HPP_
