// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2016 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#ifndef JUBATUS_CORE_CLASSIFIER_INVERTED_INDEX_CLASSIFIER_HPP_
#define JUBATUS_CORE_CLASSIFIER_INVERTED_INDEX_CLASSIFIER_HPP_

#include <stdint.h>


#include <map>
#include <string>
#include <vector>
#include "jubatus/util/math/random.h"
#include "jubatus/util/concurrent/lock.h"
#include "jubatus/util/concurrent/mutex.h"
#include "jubatus/util/concurrent/rwmutex.h"

#include "../common/type.hpp"
#include "../framework/packer.hpp"
#include "../framework/mixable.hpp"
#include "../unlearner/unlearner_base.hpp"
#include "../storage/labels.hpp"
#include "../storage/inverted_index_storage.hpp"
#include "classifier_type.hpp"
#include "classifier_base.hpp"

namespace jubatus {
namespace core {
namespace classifier {

class inverted_index_classifier : public classifier_base {
 public:
  inverted_index_classifier(size_t k, float alpha);
  void train(const common::sfv_t& fv, const std::string& label);
  void classify_with_scores(
      const common::sfv_t& fv, classify_result& scores);

  void set_label_unlearner(
      jubatus::util::lang::shared_ptr<unlearner::unlearner_base>
      label_unlearner);

  bool delete_label(const std::string& label);
  labels_t get_labels() const;
  bool set_label(const std::string& label);

  std::string name() const;

  void get_status(std::map<std::string, std::string>& status) const;

  void pack(framework::packer& pk) const;
  void unpack(msgpack::object o);
  void clear();

  std::vector<framework::mixable*> get_mixables();

 protected:
  jubatus::util::lang::shared_ptr<storage::mixable_inverted_index_storage>
      mixable_storage_;
  // A map from label to number of records that belongs to the label.
  storage::mixable_labels labels_;
  size_t k_;
  float alpha_;
  mutable jubatus::util::concurrent::rw_mutex storage_mutex_;
  jubatus::util::concurrent::mutex rand_mutex_;
  jubatus::util::math::random::mtrand rand_;
};

}  // namespace classifier
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_CLASSIFIER_INVERTED_INDEX_CLASSIFIER_HPP_
