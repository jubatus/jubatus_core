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

#ifndef JUBATUS_CORE_CLASSIFIER_CLASSIFIER_CONFIG_HPP_
#define JUBATUS_CORE_CLASSIFIER_CLASSIFIER_CONFIG_HPP_

#include "jubatus/util/data/serialization.h"
#include "jubatus/util/data/optional.h"
#include "jubatus/util/lang/shared_ptr.h"
#include "../unlearner/unlearner_config.hpp"

namespace jubatus {
namespace core {
namespace classifier {

struct classifier_config_base {
  std::string method_;
  virtual ~classifier_config_base() {}
  classifier_config_base(const std::string& method)
    : method_(method) {
  }

  template<typename Ar>
  void serialize(Ar& ar) {
    ar & JUBA_NAMED_MEMBER("method", method_);
  }
};

namespace detail {
struct classifier_parameter : public classifier_config_base {
  classifier_parameter(const std::string& method)
      : classifier_config_base(method),
        regularization_weight(1.0f) {
  }
  float regularization_weight;

  template<typename Ar>
  void serialize(Ar& ar) {
    classifier_config_base::serialize(ar);
    ar & JUBA_MEMBER(regularization_weight);
  }
};

struct unlearning_classifier_config : public classifier_parameter {
  unlearning_classifier_config(const std::string& method,
                               const common::jsonconfig::config& param)
    : classifier_parameter(method) {
    // TODO
    if (param.type() == jubatus::util::text::json::json::Null) {
      throw JUBATUS_EXCEPTION(
          common::config_exception() << common::exception::error_message(
              "parameter block is not specified in config"));
    }
  }
  util::lang::shared_ptr<unlearner::unlearner_config_base> unlearner_config_;

  template<typename Ar>
  void serialize(Ar& ar) {
    classifier_config_base::serialize(ar);
    unlearner_config_->serialize(ar);
  }
};

struct nearest_neighbor_classifier_config : public classifier_config_base {
  std::string method;
  int nearest_neighbor_num;
  float local_sensitivity;
  util::lang::shared_ptr<unlearner::unlearner_config_base> unlearner_config_;
  nearest_neighbor_classifier_config(const std::string& method,
                                     const common::jsonconfig::config& param)
    : classifier_config_base(method) {
    // TODO
  }

  template<typename Ar>
  void serialize(Ar& ar) {
    classifier_config_base::serialize(ar);
    ar & JUBA_MEMBER(method)
        & JUBA_MEMBER(nearest_neighbor_num)
        & JUBA_MEMBER(local_sensitivity);
    if (unlearner_config_) {
      unlearner_config_->serialize(ar);
    }
  }
};

}  // namespace detail

struct classifier_config {
  util::lang::shared_ptr<classifier_config_base> conf_;
  classifier_config(const std::string& method,
                    const common::jsonconfig::config& param);
  classifier_config() {
  }
  template<typename Ar>
  void serialize(Ar& ar) {
    conf_->serialize(ar);
  }
};

}  // namespace classifier
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_CLASSIFIER_CLASSIFIER_CONFIG_HPP_
