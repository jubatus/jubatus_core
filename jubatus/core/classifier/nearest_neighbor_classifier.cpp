// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2014 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#include "nearest_neighbor_classifier.hpp"

#include <cfloat>
#include <string>
#include <vector>
#include <map>
#include <utility>
#include "../framework/mixable_versioned_table.hpp"
#include "../common/unordered_map.hpp"
#include "../storage/column_table.hpp"
#include "jubatus/util/concurrent/lock.h"
#include "nearest_neighbor_classifier_util.hpp"
#include "jubatus/util/lang/bind.h"
#include "jubatus/util/lang/function.h"

using jubatus::util::lang::shared_ptr;
using jubatus::util::concurrent::scoped_lock;

namespace jubatus {
namespace core {
namespace classifier {

class nearest_neighbor_classifier::unlearning_callback {
 public:
  explicit unlearning_callback(nearest_neighbor_classifier* classifier)
      : classifier_(classifier) {
  }

  void operator()(const std::string& id) {
    classifier_->unlearn_id(id);
    classifier_->decrement_label_counter(get_label_from_id(id));
  }

 private:
  nearest_neighbor_classifier* classifier_;
};

nearest_neighbor_classifier::nearest_neighbor_classifier(
    shared_ptr<nearest_neighbor::nearest_neighbor_base> engine,
    size_t k,
    float alpha)
    : nearest_neighbor_engine_(engine), k_(k), alpha_(alpha) {
  if (!(alpha >= 0)) {
    throw JUBATUS_EXCEPTION(common::invalid_parameter(
        "local_sensitivity should >= 0"));
  }
  dynamic_cast<framework::mixable_versioned_table*>
      (nearest_neighbor_engine_->get_mixable())->set_update_callback(
          util::lang::bind(
              &nearest_neighbor_classifier::regenerate_label_counter, this));
}

void nearest_neighbor_classifier::train(
    const common::sfv_t& fv, const std::string& label) {
  std::string id;
  {
    util::concurrent::scoped_lock lk(rand_mutex_);
    id = make_id_from_label(label, rand_);
  }
  if (unlearner_) {
    util::concurrent::scoped_lock unlearner_lk(unlearner_mutex_);

    // acquire the lock outside of touch() function
    shared_ptr<storage::column_table> table =
        nearest_neighbor_engine_->get_table();
    util::concurrent::scoped_wlock lk(table->get_mutex());

    if (!unlearner_->touch(id)) {
      throw JUBATUS_EXCEPTION(common::exception::runtime_error(
          "cannot add new ID as number of sticky IDs reached "
          "the maximum size of unlearner: " + id));
    }
  }

  // unlearner is not called in set_row
  nearest_neighbor_engine_->set_row(id, fv);  // lock acquired inside
  set_label(label);
  labels_.increment(label);
}

void nearest_neighbor_classifier::set_label_unlearner(
    shared_ptr<unlearner::unlearner_base> label_unlearner) {
  label_unlearner->set_callback(unlearning_callback(this));
  unlearner_ = label_unlearner;
  // Support unlearning in MIX
  dynamic_cast<framework::mixable_versioned_table*>
      (nearest_neighbor_engine_->get_mixable())->set_unlearner(unlearner_);
}

std::string nearest_neighbor_classifier::classify(
    const common::sfv_t& fv) const {
  classify_result result;
  classify_with_scores(fv, result);
  float max_score = -FLT_MAX;
  std::string max_class;
  for (std::vector<classify_result_elem>::const_iterator it = result.begin();
      it != result.end(); ++it) {
    if (it == result.begin() || it->score > max_score) {
      max_score = it->score;
      max_class = it->label;
    }
  }
  return max_class;
}

void nearest_neighbor_classifier::classify_with_scores(
    const common::sfv_t& fv, classify_result& scores) const {
  std::vector<std::pair<std::string, float> > ids;

  // lock acquired inside
  nearest_neighbor_engine_->neighbor_row(fv, ids, k_);
  const labels_t labels = labels_.get_labels();

  std::map<std::string, float> m;
  for (labels_t::const_iterator iter = labels.begin();
       iter != labels.end(); ++iter) {
    m.insert(std::make_pair(iter->first, 0));
  }

  for (size_t i = 0; i < ids.size(); ++i) {
    std::string label = get_label_from_id(ids[i].first);
    m[label] += std::exp(-alpha_ * ids[i].second);
  }

  scores.clear();
  for (std::map<std::string, float>::const_iterator iter = m.begin();
       iter != m.end(); ++iter) {
    classify_result_elem elem(iter->first, iter->second);
    scores.push_back(elem);
  }
}

bool nearest_neighbor_classifier::delete_label(const std::string& label) {
  if (!labels_.erase(label)) {
    return false;
  }

  shared_ptr<storage::column_table> table =
      nearest_neighbor_engine_->get_table();

  util::concurrent::scoped_wlock lk(table->get_mutex());

  std::vector<std::string> ids_to_be_deleted;
  for (size_t i = 0, n = table->size_nolock(); i < n; ++i) {
    std::string id = table->get_key_nolock(i);
    std::string l = get_label_from_id(id);
    if (l == label) {
      ids_to_be_deleted.push_back(id);
    }
  }

  for (size_t i = 0, n = ids_to_be_deleted.size(); i < n; ++i) {
    const std::string& id = ids_to_be_deleted[i];
    table->delete_row_nolock(id);
    if (unlearner_) {
      util::concurrent::scoped_lock unlearner_lk(unlearner_mutex_);
      unlearner_->remove(id);
    }
  }

  return true;
}

void nearest_neighbor_classifier::clear() {
  nearest_neighbor_engine_->clear();  // lock acquired inside
  labels_.clear();
  if (unlearner_) {
    util::concurrent::scoped_lock unlearner_lk(unlearner_mutex_);
    unlearner_->clear();
  }
}

labels_t nearest_neighbor_classifier::get_labels() const {
  return labels_.get_labels();
}

bool nearest_neighbor_classifier::set_label(const std::string& label) {
  return labels_.add(label);
}

std::string nearest_neighbor_classifier::name() const {
  return "nearest_neighbor_classifier:" + nearest_neighbor_engine_->type();
}

void nearest_neighbor_classifier::get_status(
    std::map<std::string, std::string>& status) const {
  // unimplemented
}

void nearest_neighbor_classifier::pack(framework::packer& pk) const {
  pk.pack_array(2);
  nearest_neighbor_engine_->pack(pk);
  labels_.pack(pk);
}

void nearest_neighbor_classifier::unpack(msgpack::object o) {
  if (o.type != msgpack::type::ARRAY || o.via.array.size != 2) {
    throw msgpack::type_error();
  }
  nearest_neighbor_engine_->unpack(o.via.array.ptr[0]);
  labels_.unpack(o.via.array.ptr[1]);
}

std::vector<framework::mixable*> nearest_neighbor_classifier::get_mixables() {
  std::vector<framework::mixable*> mixables;
  mixables.push_back(nearest_neighbor_engine_->get_mixable());
  return mixables;
}

void nearest_neighbor_classifier::unlearn_id(const std::string& id) {
  // This method must be called via touch() function.
  // touch() must be done with holding lock
  // so this function must not get lock
  nearest_neighbor_engine_->get_table()->delete_row_nolock(id);
}

void nearest_neighbor_classifier::decrement_label_counter(
    const std::string& label) {
  labels_.decrement(label);
}

void nearest_neighbor_classifier::regenerate_label_counter() {
  labels_t new_labels;

  // Copy keyset of labels_ to new labels.
  // labels_ contains labels that registered by set_label.
  const labels_t labels_on_model = labels_.get_labels();
  for (labels_t::const_iterator iter = labels_on_model.begin();
       iter != labels_on_model.end(); ++iter) {
    new_labels[iter->first] = 0;
  }

  {
    // Count id for each label
    shared_ptr<const storage::column_table> table =
        nearest_neighbor_engine_->get_const_table();
    util::concurrent::scoped_rlock table_lk(table->get_mutex());

    for (size_t i = 0, n = table->size_nolock(); i < n; ++i) {
      std::string id = table->get_key_nolock(i);
      std::string label = get_label_from_id(id);
      new_labels[label] += 1;
    }
  }

  labels_.swap(new_labels);
}

}  // namespace classifier
}  // namespace core
}  // namespace jubatus
