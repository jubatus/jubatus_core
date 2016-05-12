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

#include "datum_to_fv_converter.hpp"

#include <cmath>
#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include "jubatus/util/data/optional.h"
#include "jubatus/util/lang/shared_ptr.h"
#include "binary_feature.hpp"
#include "combination_feature.hpp"
#include "counter.hpp"
#include "datum.hpp"
#include "exception.hpp"
#include "feature_hasher.hpp"
#include "match_all.hpp"
#include "mixable_weight_manager.hpp"
#include "num_feature.hpp"
#include "num_filter.hpp"
#include "space_splitter.hpp"
#include "string_feature.hpp"
#include "string_filter.hpp"
#include "weight_manager.hpp"
#include "without_split.hpp"

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

/// impl

class datum_to_fv_converter_impl {
 private:
  typedef jubatus::util::data::unordered_map<std::string, float> weight_t;

  struct string_filter_rule {
    jubatus::util::lang::shared_ptr<key_matcher> matcher_;
    jubatus::util::lang::shared_ptr<string_filter> filter_;
    std::string suffix_;

    void filter(const datum::sv_t& string_values, datum::sv_t& filtered) const {
      for (size_t i = 0; i < string_values.size(); ++i) {
        const std::pair<std::string, std::string>& value = string_values[i];
        if (matcher_->match(value.first)) {
          std::string out;
          filter_->filter(value.second, out);
          std::string dest = value.first + suffix_;
          filtered.push_back(std::make_pair(dest, out));
        }
      }
    }
  };

  struct num_filter_rule {
    jubatus::util::lang::shared_ptr<key_matcher> matcher_;
    jubatus::util::lang::shared_ptr<num_filter> filter_;
    std::string suffix_;

    void filter(const datum::nv_t& num_values, datum::nv_t& filtered) const {
      for (size_t i = 0; i < num_values.size(); ++i) {
        const std::pair<std::string, double>& value = num_values[i];
        if (matcher_->match(value.first)) {
          double out = filter_->filter(value.second);
          std::string dest = value.first + suffix_;
          filtered.push_back(std::make_pair(dest, out));
        }
      }
    }
  };

  struct string_feature_rule {
    std::string name_;
    jubatus::util::lang::shared_ptr<key_matcher> matcher_;
    jubatus::util::lang::shared_ptr<string_feature> splitter_;
    std::vector<splitter_weight_type> weights_;

    string_feature_rule(
        const std::string& name,
        jubatus::util::lang::shared_ptr<key_matcher> matcher,
        jubatus::util::lang::shared_ptr<string_feature> splitter,
        const std::vector<splitter_weight_type>& weights)
        : name_(name),
          matcher_(matcher),
          splitter_(splitter),
          weights_(weights) {
    }
  };

  struct num_feature_rule {
    std::string name_;
    jubatus::util::lang::shared_ptr<key_matcher> matcher_;
    jubatus::util::lang::shared_ptr<num_feature> feature_func_;

    num_feature_rule(
        const std::string& name,
        jubatus::util::lang::shared_ptr<key_matcher> matcher,
        jubatus::util::lang::shared_ptr<num_feature> feature_func)
        : name_(name),
          matcher_(matcher),
          feature_func_(feature_func) {
    }
  };

  struct binary_feature_rule {
    std::string name_;
    jubatus::util::lang::shared_ptr<key_matcher> matcher_;
    jubatus::util::lang::shared_ptr<binary_feature> feature_func_;

    binary_feature_rule(
        const std::string& name,
        jubatus::util::lang::shared_ptr<key_matcher> matcher,
        jubatus::util::lang::shared_ptr<binary_feature> feature_func)
        : name_(name),
          matcher_(matcher),
          feature_func_(feature_func) {
    }
  };

  struct combination_feature_rule {
    std::string name_;
    jubatus::util::lang::shared_ptr<key_matcher> matcher_left_;
    jubatus::util::lang::shared_ptr<key_matcher> matcher_right_;
    jubatus::util::lang::shared_ptr<combination_feature> feature_func_;

    combination_feature_rule(
        const std::string& name,
        jubatus::util::lang::shared_ptr<key_matcher> matcher_left,
        jubatus::util::lang::shared_ptr<key_matcher> matcher_right,
        jubatus::util::lang::shared_ptr<combination_feature> feature_func)
        : name_(name),
          matcher_left_(matcher_left),
          matcher_right_(matcher_right),
          feature_func_(feature_func) {
    }
  };

  // binarys
  std::vector<binary_feature_rule> binary_rules_;
  std::vector<combination_feature_rule> combination_rules_;

  std::vector<string_filter_rule> string_filter_rules_;
  std::vector<num_filter_rule> num_filter_rules_;
  std::vector<string_feature_rule> string_rules_;
  std::vector<num_feature_rule> num_rules_;

  jubatus::util::lang::shared_ptr<mixable_weight_manager> mixable_weights_;

  jubatus::util::data::optional<feature_hasher> hasher_;

 public:
  datum_to_fv_converter_impl()
    : mixable_weights_(
        new mixable_weight_manager(
            jubatus::util::lang::shared_ptr<weight_manager>(
                new weight_manager))) {
  }

  void clear_rules() {
    string_filter_rules_.clear();
    num_filter_rules_.clear();
    string_rules_.clear();
    num_rules_.clear();
    binary_rules_.clear();
    combination_rules_.clear();
  }

  void register_string_filter(
      jubatus::util::lang::shared_ptr<key_matcher> matcher,
      jubatus::util::lang::shared_ptr<string_filter> filter,
      const std::string& suffix) {
    string_filter_rule rule = { matcher, filter, suffix };
    string_filter_rules_.push_back(rule);
  }

  void register_num_filter(
      jubatus::util::lang::shared_ptr<key_matcher> matcher,
      jubatus::util::lang::shared_ptr<num_filter> filter,
      const std::string& suffix) {
    num_filter_rule rule = { matcher, filter, suffix };
    num_filter_rules_.push_back(rule);
  }

  void register_string_rule(
      const std::string& name,
      jubatus::util::lang::shared_ptr<key_matcher> matcher,
      jubatus::util::lang::shared_ptr<string_feature> splitter,
      const std::vector<splitter_weight_type>& weights) {
    string_rules_.push_back(
       string_feature_rule(name, matcher, splitter, weights));
  }

  void register_num_rule(
      const std::string& name,
      jubatus::util::lang::shared_ptr<key_matcher> matcher,
      jubatus::util::lang::shared_ptr<num_feature> feature_func) {
    num_rules_.push_back(num_feature_rule(name, matcher, feature_func));
  }

  void register_binary_rule(
      const std::string& name,
      jubatus::util::lang::shared_ptr<key_matcher> matcher,
      jubatus::util::lang::shared_ptr<binary_feature> feature_func) {
    binary_rules_.push_back(binary_feature_rule(name, matcher, feature_func));
  }

  void register_combination_rule(
      const std::string& name,
      jubatus::util::lang::shared_ptr<key_matcher> matcher_left,
      jubatus::util::lang::shared_ptr<key_matcher> matcher_right,
      jubatus::util::lang::shared_ptr<combination_feature> feature_func) {
    combination_rules_.push_back(
        combination_feature_rule(
            name,
            matcher_left,
            matcher_right,
            feature_func));
  }

  void add_weight(const std::string& key, float weight) {
    jubatus::util::lang::shared_ptr<weight_manager> weights =
        mixable_weights_->get_model();
    if (weights) {
      (*weights).add_weight(key, weight);
    }
  }

  /**
   * Converts the given Datum to Sparse Feature Vector.
   * This is a `const` function that does not update weight_manager.
   */
  void convert(const datum& datum, common::sfv_t& ret_fv) const {
    common::sfv_t fv;

    // Filter and convert string values (do not update weight).
    std::vector<std::pair<std::string, std::string> >
        string_records(datum.string_values_);
    std::vector<std::pair<std::string, std::string> > filtered_string_records;
    filter_strings(datum.string_values_, filtered_string_records);
    string_records.insert(
        string_records.end(),
        filtered_string_records.begin(),
        filtered_string_records.end());
    convert_strings(string_records, fv);

    // Conversion process other than string values.
    convert_common(datum, fv);

    fv.swap(ret_fv);
  }

  /**
   * Converts the given Datum to Sparse Feature Vector.
   * This is a `non-const` function that updates weight_manager.
   */
  void convert_and_update_weight(const datum& datum, common::sfv_t& ret_fv) {
    common::sfv_t fv;

    // Filter and convert string values (updates weight).
    std::vector<std::pair<std::string, std::string> >
        string_records(datum.string_values_);
    std::vector<std::pair<std::string, std::string> > filtered_string_records;
    filter_strings(datum.string_values_, filtered_string_records);
    string_records.insert(
        string_records.end(),
        filtered_string_records.begin(),
        filtered_string_records.end());
    convert_strings_and_update_weight(string_records, fv);

    // Conversion process other than string values.
    convert_common(datum, fv);

    fv.swap(ret_fv);
  }

  void convert_common(const datum& datum, common::sfv_t& fv) const {
    // Filter & Convert Numeric Values
    std::vector<std::pair<std::string, double> > filtered_nums;
    filter_nums(datum.num_values_, filtered_nums);
    convert_nums(datum.num_values_, fv);
    convert_nums(filtered_nums, fv);

    // Convert Binary Values
    convert_binaries(datum.binary_values_, fv);

    // Remove dimension whose value is 0.
    fv.erase(remove_if(fv.begin(), fv.end(), is_zero()), fv.end());

    // Compute Combinations
    convert_combinations(fv);

    // Hash Feature Vector Keys
    if (hasher_) {
      hasher_->hash_feature_keys(fv);
    }
  }

  void revert_feature(
      const std::string& feature,
      std::pair<std::string, std::string>& expect) const {
    // format of string feature is
    // "<KEY_NAME>$<VALUE>@<FEATURE_TYPE>#<SAMPLE_WEIGHT>/<GLOBAL_WEIGHT>"
    size_t sharp = feature.rfind('#');
    if (sharp == std::string::npos) {
      throw JUBATUS_EXCEPTION(
          converter_exception("this feature is not string feature"));
    }
    size_t at = feature.rfind('@', sharp);
    if (at == std::string::npos) {
      throw JUBATUS_EXCEPTION(
          converter_exception("this feature is not valid feature"));
    }
    size_t dollar = feature.rfind('$', at);
    if (dollar == std::string::npos) {
      throw JUBATUS_EXCEPTION(
          converter_exception("this feature is not valid feature"));
    }
    if (feature.substr(at + 1, sharp - at - 1) != "str") {
      throw JUBATUS_EXCEPTION(
          converter_exception("this feature is not revertible"));
    }

    std::string key(feature.substr(0, dollar));
    std::string value(feature.substr(dollar + 1, at - dollar - 1));

    expect.first.swap(key);
    expect.second.swap(value);
  }

  void set_hash_max_size(uint64_t hash_max_size) {
    hasher_ = feature_hasher(hash_max_size);
  }

  void set_weight_manager(jubatus::util::lang::shared_ptr<weight_manager> wm) {
    mixable_weights_->set_model(wm);
  }

  void clear_weights() {
    jubatus::util::lang::shared_ptr<weight_manager> weights =
        mixable_weights_->get_model();
    if (weights) {
      weights->clear();
    }
  }

 private:
  void filter_strings(
      const datum::sv_t& string_values,
      datum::sv_t& filtered_values) const {
    for (size_t i = 0; i < string_filter_rules_.size(); ++i) {
      datum::sv_t update;
      string_filter_rules_[i].filter(string_values, update);
      string_filter_rules_[i].filter(filtered_values, update);

      filtered_values.insert(filtered_values.end(), update.begin(),
                             update.end());
    }
  }

  void filter_nums(
      const datum::nv_t& num_values,
      datum::nv_t& filtered_values) const {
    for (size_t i = 0; i < num_filter_rules_.size(); ++i) {
      datum::nv_t update;
      num_filter_rules_[i].filter(num_values, update);
      num_filter_rules_[i].filter(filtered_values, update);

      filtered_values.insert(
          filtered_values.end(), update.begin(), update.end());
    }
  }

  void convert_strings(
      const datum::sv_t& string_values,
      common::sfv_t& ret_fv) const {
    jubatus::util::lang::shared_ptr<weight_manager> weights =
        mixable_weights_->get_model();

    for (size_t i = 0; i < string_rules_.size(); ++i) {
      const string_feature_rule& splitter = string_rules_[i];

      for (size_t j = 0; j < string_values.size(); ++j) {
        const std::string& key = string_values[j].first;
        const std::string& value = string_values[j].second;

        if (!splitter.matcher_->match(key)) {
          continue;
        }

        // Extract features from string (using splitter) and count its term
        // frequency (TF).
        counter<std::string> count;
        count_words(splitter, value, count);

        for (size_t k = 0; k < splitter.weights_.size(); ++k) {
          // Extracted features are weighted by (sample_weight * global_weight)
          // and added to the resulting feature vector (ret_fv).
          weights->add_string_features(
              key,
              splitter.name_,
              splitter.weights_[k],
              count,
              ret_fv);
        }
      }
    }
  }

  bool contains_idf(const string_feature_rule& s) const {
    for (size_t i = 0; i < s.weights_.size(); ++i) {
      if (s.weights_[i].term_weight_type_ == IDF) {
        return true;
      }
    }
    return false;
  }

  void convert_strings_and_update_weight(
      const datum::sv_t& string_values,
      common::sfv_t& ret_fv) {
    jubatus::util::lang::shared_ptr<weight_manager> weights =
        mixable_weights_->get_model();

    // Increment document count (number of datum processed).
    weights->increment_document_count();

    for (size_t i = 0; i < string_rules_.size(); ++i) {
      const string_feature_rule& splitter = string_rules_[i];

      for (size_t j = 0; j < string_values.size(); ++j) {
        const std::string& key = string_values[j].first;
        const std::string& value = string_values[j].second;

        if (!splitter.matcher_->match(key)) {
          continue;
        }

        // Extract features from string (using splitter) and count its term
        // frequency (TF).
        counter<std::string> count;
        count_words(splitter, value, count);

        for (size_t k = 0; k < splitter.weights_.size(); ++k) {
          // Update the weights.
          weights->update_weight(
              key,
              splitter.name_,
              splitter.weights_[k],
              count);

          // Extracted features are weighted by (sample_weight * global_weight)
          // and added to the resulting feature vector (ret_fv).
          weights->add_string_features(
              key,
              splitter.name_,
              splitter.weights_[k],
              count,
              ret_fv);
        }
      }
    }
  }

  void convert_binaries(
      const datum::sv_t& binary_values,
      common::sfv_t& ret_fv) const {
    for (size_t i = 0; i < binary_rules_.size(); ++i) {
      convert_binaries(binary_rules_[i], binary_values, ret_fv);
    }
  }

  void convert_binaries(
      const binary_feature_rule& feature,
      const datum::sv_t& binary_values,
      common::sfv_t& ret_fv) const {
    for (size_t j = 0; j < binary_values.size(); ++j) {
      const std::string& key = binary_values[j].first;
      const std::string& value = binary_values[j].second;
      if (feature.matcher_->match(key)) {
        feature.feature_func_->add_feature(
            make_binary_feature_name(key, feature.name_),
            value,
            ret_fv);
      }
    }
  }

  /**
   * Split the string `value` using the `splitter` feature extractor and
   * compute the term frequency.
   */
  void count_words(
      const string_feature_rule& splitter,
      const std::string& value,
      counter<std::string>& counter) const {
    std::vector<string_feature_element> elements;
    splitter.splitter_->extract(value, elements);

    for (size_t i = 0; i < elements.size(); i++) {
      counter[elements[i].value] += elements[i].score;
    }
  }

  void convert_nums(
      const datum::nv_t& num_values,
      common::sfv_t& ret_fv) const {
    for (size_t i = 0; i < num_values.size(); ++i) {
      convert_num(num_values[i].first, num_values[i].second, ret_fv);
    }
  }

  void convert_num(
      const std::string& key,
      double value,
      common::sfv_t& ret_fv) const {
    for (size_t i = 0; i < num_rules_.size(); ++i) {
      const num_feature_rule& r = num_rules_[i];
      if (r.matcher_->match(key)) {
        r.feature_func_->add_feature(
            make_num_feature_name(key, r.name_),
            value,
            ret_fv);
      }
    }
  }

  void convert_combinations(common::sfv_t& ret_fv) const {
    const size_t original_size = ret_fv.size();

    if (original_size < 2) {
      // Must have at least 2 features to generate combinations.
      return;
    }

    for (size_t i = 0; i < combination_rules_.size(); ++i) {
      const combination_feature_rule& r = combination_rules_[i];
      if (r.feature_func_->is_commutative()) {
        for (size_t j = 0 ; j < original_size - 1; ++j) {
          for (size_t m = j + 1; m < original_size; ++m) {
            if ((r.matcher_left_->match(ret_fv[j].first)
                  && r.matcher_right_->match(ret_fv[m].first)) ||
                (r.matcher_right_->match(ret_fv[j].first)
                  && r.matcher_left_->match(ret_fv[m].first))) {
              r.feature_func_->add_feature(
                  ret_fv[j].first + "&" + ret_fv[m].first + "/" + r.name_,
                  ret_fv[j].second,
                  ret_fv[m].second,
                  ret_fv);
            }
          }
        }
      } else {
        for (size_t j = 0 ; j < original_size; ++j) {
          for (size_t m = 0; m < original_size; ++m) {
            if (j == m) {
              continue;
            }
            if (r.matcher_left_->match(ret_fv[j].first)
                  && r.matcher_right_->match(ret_fv[m].first)) {
              r.feature_func_->add_feature(
                  ret_fv[j].first + "&" + ret_fv[m].first + "/" + r.name_,
                  ret_fv[j].second,
                  ret_fv[m].second,
                  ret_fv);
            }
          }
        }
      }
    }
  }
};

datum_to_fv_converter::datum_to_fv_converter()
    : pimpl_(new datum_to_fv_converter_impl()) {
}

datum_to_fv_converter::~datum_to_fv_converter() {
}

void datum_to_fv_converter::convert(
    const datum& datum,
    common::sfv_t& ret_fv) const {
  pimpl_->convert(datum, ret_fv);
}

void datum_to_fv_converter::convert_and_update_weight(
    const datum& datum,
    common::sfv_t& ret_fv) {
  pimpl_->convert_and_update_weight(datum, ret_fv);
}

void datum_to_fv_converter::clear_rules() {
  pimpl_->clear_rules();
}

void datum_to_fv_converter::register_string_filter(
    jubatus::util::lang::shared_ptr<key_matcher> matcher,
    jubatus::util::lang::shared_ptr<string_filter> filter,
    const std::string& suffix) {
  pimpl_->register_string_filter(matcher, filter, suffix);
}

void datum_to_fv_converter::register_num_filter(
    jubatus::util::lang::shared_ptr<key_matcher> matcher,
    jubatus::util::lang::shared_ptr<num_filter> filter,
    const std::string& suffix) {
  pimpl_->register_num_filter(matcher, filter, suffix);
}

void datum_to_fv_converter::register_string_rule(
    const std::string& name,
    jubatus::util::lang::shared_ptr<key_matcher> matcher,
    jubatus::util::lang::shared_ptr<string_feature> splitter,
    const std::vector<splitter_weight_type>& weights) {
  pimpl_->register_string_rule(name, matcher, splitter, weights);
}

void datum_to_fv_converter::register_num_rule(
    const std::string& name,
    jubatus::util::lang::shared_ptr<key_matcher> matcher,
    jubatus::util::lang::shared_ptr<num_feature> feature_func) {
  pimpl_->register_num_rule(name, matcher, feature_func);
}

void datum_to_fv_converter::register_binary_rule(
    const std::string& name,
    jubatus::util::lang::shared_ptr<key_matcher> matcher,
    jubatus::util::lang::shared_ptr<binary_feature> feature_func) {
  pimpl_->register_binary_rule(name, matcher, feature_func);
}

void datum_to_fv_converter::register_combination_rule(
    const std::string& name,
    jubatus::util::lang::shared_ptr<key_matcher> matcher_left,
    jubatus::util::lang::shared_ptr<key_matcher> matcher_right,
    jubatus::util::lang::shared_ptr<combination_feature> feature_func) {
  pimpl_->register_combination_rule(
      name,
      matcher_left,
      matcher_right,
      feature_func);
}

void datum_to_fv_converter::add_weight(const std::string& key, float weight) {
  pimpl_->add_weight(key, weight);
}

void datum_to_fv_converter::revert_feature(
    const std::string& feature,
    std::pair<std::string, std::string>& expect) const {
  pimpl_->revert_feature(feature, expect);
}

void datum_to_fv_converter::set_hash_max_size(uint64_t hash_max_size) {
  pimpl_->set_hash_max_size(hash_max_size);
}

void datum_to_fv_converter::set_weight_manager(
    jubatus::util::lang::shared_ptr<weight_manager> wm) {
  pimpl_->set_weight_manager(wm);
}

void datum_to_fv_converter::clear_weights() {
  pimpl_->clear_weights();
}

frequency_weight_type get_frequency_weight_type(const std::string& name) {
  if (name == "bin") {
    return FREQ_BINARY;
  } else if (name == "tf") {
    return TERM_FREQUENCY;
  } else if (name == "log_tf") {
    return LOG_TERM_FREQUENCY;
  } else {
    throw JUBATUS_EXCEPTION(
        converter_exception("unknown sample_weight: [" + name + "]"));
  }
}

std::string get_frequency_weight_name(frequency_weight_type type) {
  switch (type) {
    case FREQ_BINARY:
      return "bin";
    case TERM_FREQUENCY:
      return "tf";
    case LOG_TERM_FREQUENCY:
      return "log_tf";
    default:  // this shouldn't happen
      throw JUBATUS_EXCEPTION(converter_exception(
          "unknown frequency_weight_type: [" +
          lexical_cast<std::string>(type) + "]"));
  }
}

term_weight_type get_term_weight_type(const std::string& name) {
  if (name == "bin") {
    return TERM_BINARY;
  } else if (name == "idf") {
    return IDF;
  } else if (name == "weight") {
    return WITH_WEIGHT_FILE;
  } else {
    throw JUBATUS_EXCEPTION(
        converter_exception("unknown global_weight: [" + name + "]"));
  }
}

std::string get_term_weight_name(term_weight_type type) {
  switch (type) {
    case TERM_BINARY:
      return "bin";
    case IDF:
      return "idf";
    case WITH_WEIGHT_FILE:
      return "weight";
    default:  // this shouldn't happen
      throw JUBATUS_EXCEPTION(converter_exception(
          "unknown term_weight_type: [" +
          lexical_cast<std::string>(type) + "]"));
  }
}

/**
 * Checks if the datum key has a valid name.
 */
void check_key(const std::string& key) {
  if (key.find('$') != std::string::npos) {
    throw JUBATUS_EXCEPTION(
        converter_exception("feature key cannot contain '$': " + key));
  }
}

std::string make_string_feature_name(
    const std::string& key,
    const std::string& value,
    const std::string& type,
    frequency_weight_type sample_weight,
    term_weight_type global_weight) {
  check_key(key);
  return key + "$" + value + "@" + type + "#" +
         get_frequency_weight_name(sample_weight) + "/" +
         get_term_weight_name(global_weight);
}

std::string make_num_feature_name(
    const std::string& key,
    const std::string& type) {
  check_key(key);
  return key + "@" + type;
}

std::string make_binary_feature_name(
    const std::string& key,
    const std::string& type) {
  check_key(key);
  return key;
}

std::string make_weight_name(
    const std::string& key,
    const std::string& value,
    const std::string& type) {
  check_key(key);
  return key + "$" + value + "@" + type;
}

}  // namespace fv_converter
}  // namespace core
}  // namespace jubatus
