// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2014 Preferred Infrastructure and Nippon Telegraph and Telephone Corporation.
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

#ifndef JUBATUS_CORE_CLASSIFIER_NEAREST_NEIGHBOR_CLASSIFIER_HPP_
#define JUBATUS_CORE_CLASSIFIER_NEAREST_NEIGHBOR_CLASSIFIER_HPP_

#include <stdint.h>

#include <map>
#include <string>
#include <vector>
#include "jubatus/util/math/random.h"
#include "jubatus/util/data/unordered_set.h"

#include "../common/type.hpp"
#include "../nearest_neighbor/nearest_neighbor_base.hpp"
#include "../unlearner/unlearner_base.hpp"
#include "classifier_type.hpp"
#include "classifier_base.hpp"

namespace jubatus {
namespace core {
namespace classifier {

class nearest_neighbor_classifier : public classifier_base {
 public:
  nearest_neighbor_classifier(
      jubatus::util::lang::shared_ptr<nearest_neighbor::nearest_neighbor_base>
          nearest_neighbor_engine,
      size_t k);

  void train(const common::sfv_t& fv, const std::string& label);

  void set_label_unlearner(
      jubatus::util::lang::shared_ptr<unlearner::unlearner_base>
          label_unlearner);

  std::string classify(const common::sfv_t& fv) const;
  void classify_with_scores(const common::sfv_t& fv,
                            classify_result& scores) const;
  bool delete_label(const std::string& label);
  void clear();

  std::vector<std::string> get_labels() const;
  bool set_label(const std::string& label);

  std::string name() const;

  void get_status(std::map<std::string, std::string>& status) const;

  void pack(framework::packer& pk) const;
  void unpack(msgpack::object o);

  framework::mixable* get_mixable();

 private:
  jubatus::util::lang::shared_ptr<nearest_neighbor::nearest_neighbor_base>
      nearest_neighbor_engine_;
  jubatus::util::data::unordered_set<std::string> labels_;
  size_t k_;
  jubatus::util::lang::shared_ptr<unlearner::unlearner_base> unlearner_;
  jubatus::util::math::random::mtrand rand_;

  class unlearning_callback;
  void unlearn_id(const std::string& id);
};

}  // namespace classifier
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_CLASSIFIER_NEAREST_NEIGHBOR_CLASSIFIER_HPP_
