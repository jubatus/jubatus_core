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

#ifndef JUBATUS_CORE_CLASSIFIER_CLASSIFIER_BASE_HPP_
#define JUBATUS_CORE_CLASSIFIER_CLASSIFIER_BASE_HPP_

#include <stdint.h>

#include <map>
#include <string>
#include <vector>

#include "../common/type.hpp"
#include "../framework/packer.hpp"
#include "../framework/mixable.hpp"
#include "../unlearner/unlearner_base.hpp"
#include "classifier_type.hpp"

namespace jubatus {
namespace core {
namespace classifier {

class classifier_base {
 public:
  classifier_base() {
  }
  virtual ~classifier_base() {
  }

  virtual void train(const common::sfv_t& fv, const std::string& label) = 0;

  virtual void classify_with_scores(
      const common::sfv_t& fv, classify_result& scores) const = 0;

  virtual void set_label_unlearner(
      jubatus::util::lang::shared_ptr<unlearner::unlearner_base>
          label_unlearner) = 0;

  virtual bool delete_label(const std::string& label) = 0;
  virtual labels_t get_labels() const = 0;
  virtual bool set_label(const std::string& label) = 0;

  virtual std::string name() const = 0;

  virtual void get_status(std::map<std::string, std::string>& status) const = 0;

  virtual void pack(framework::packer& pk) const = 0;
  virtual void unpack(msgpack::object o) = 0;
  virtual void clear() = 0;

  virtual std::vector<framework::mixable*> get_mixables() = 0;
};

}  // namespace classifier
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_CLASSIFIER_CLASSIFIER_BASE_HPP_
