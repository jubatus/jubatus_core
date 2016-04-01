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
#include "recommender_classifier.hpp"
#include "jubatus/util/concurrent/lock.h"

using jubatus::util::lang::shared_ptr;
using jubatus::util::data::unordered_set;
using jubatus::util::concurrent::scoped_lock;

namespace jubatus {
namespace core {
namespace classifier {

namespace {
std::string make_id_from_label(
    const std::string& label,
    jubatus::util::math::random::mtrand& rand) {
  const size_t n = 8;
  std::string result = label;
  result.reserve(label.size() + 1 + n);
  result.push_back('_');
  for (size_t i = 0; i < n; ++i) {
    int r = rand.next_int(26 * 2 + 10);
    if (r < 26) {
      result.push_back('a' + r);
    } else if (r < 26 * 2) {
      result.push_back('A' + (r - 26));
    } else {
      result.push_back('0' + (r - 26 * 2));
    }
  }
  return result;
}

std::string get_label_from_id(const std::string& id) {
  size_t pos = id.find_last_of("_");
  return id.substr(0, pos);
}
}  // namespace

class recommender_classifier::unlearning_callback {
 public:
  explicit unlearning_callback(recommender_classifier* classifier)
      : classifier_(classifier) {
  }

  void operator()(const std::string& id) {
    classifier_->unlearn_id(id);
  }

 private:
  recommender_classifier* classifier_;
};

recommender_classifier::recommender_classifier(
    shared_ptr<recommender::recommender_base> engine,
    size_t k,
    float alpha)
    : recommender_engine_(engine), k_(k), alpha_(alpha) {
  if (!(alpha >= 0)) {
    throw JUBATUS_EXCEPTION(common::invalid_parameter(
        "local_sensitivity should >= 0"));
  }
}

void recommender_classifier::train(
    const common::sfv_t& fv, 
    const std::string& label) {
  std::string id;
  {
    util::concurrent::scoped_lock lk(rand_mutex_);
    id = make_id_from_label(label, rand_);
  }
  recommender_engine_->update_row(id, fv);
  set_label(label);
}

void recommender_classifier::set_label_unlearner(
    shared_ptr<unlearner::unlearner_base> label_unlearner) {
  label_unlearner->set_callback(unlearning_callback(this));
  unlearner_ = label_unlearner;
}

std::string recommender_classifier::classify(
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

void recommender_classifier::classify_with_scores(
    const common::sfv_t& fv, classify_result& scores) const {
  std::vector<std::pair<std::string, float> > ids;
  recommender_engine_->neighbor_row(fv, ids, k_);

  std::map<std::string, float> m;
  {
    util::concurrent::scoped_lock lk(label_mutex_);
    for (unordered_set<std::string>::const_iterator iter = labels_.begin();
         iter != labels_.end(); ++iter) {
      m.insert(std::make_pair(*iter, 0));
    }
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

bool recommender_classifier::delete_label(const std::string& label) {
  {
    util::concurrent::scoped_lock lk(label_mutex_);
    if (labels_.erase(label) == 0) {
      return false;
    }
  }

  std::vector<std::string> ids;
  recommender_engine_->get_all_row_ids(ids);

  std::vector<std::string> ids_to_be_deleted;
  std::vector<std::string>::const_iterator iter;
  for (iter = ids.begin(); iter != ids.end(); iter++) {
    std::string l = get_label_from_id(*iter);
    if (l == label) {
      ids_to_be_deleted.push_back(*iter);
    }
  }

  for (size_t i = 0, n = ids_to_be_deleted.size(); i < n; ++i) {
    const std::string& id = ids_to_be_deleted[i];
    recommender_engine_->clear_row(id); 
  }
  
  return true;
}

void recommender_classifier::clear() {
  recommender_engine_->clear();
  {
    util::concurrent::scoped_lock lk(label_mutex_);
    labels_.clear();
  }
  if (unlearner_) {
    unlearner_->clear();
  }
}

std::vector<std::string> recommender_classifier::get_labels() const {
  util::concurrent::scoped_lock lk(label_mutex_);
  std::vector<std::string> result;
  for (unordered_set<std::string>::const_iterator iter = labels_.begin();
       iter != labels_.end(); ++iter) {
    result.push_back(*iter);
  }
  return result;
}

bool recommender_classifier::set_label(const std::string& label) {
  util::concurrent::scoped_lock lk(label_mutex_);
  return labels_.insert(label).second;
}

std::string recommender_classifier::name() const {
  return "recommender_classifier:" + recommender_engine_->type();
}

void recommender_classifier::get_status(
    std::map<std::string, std::string>& status) const {
  // unimplemented
}

void recommender_classifier::pack(framework::packer& pk) const {
  pk.pack_array(2);
  recommender_engine_->pack(pk);

  util::concurrent::scoped_lock lk(label_mutex_);
  pk.pack_array(labels_.size());
  for (unordered_set<std::string>::const_iterator iter = labels_.begin();
       iter != labels_.end(); ++iter) {
    pk.pack(*iter);
  }
}

void recommender_classifier::unpack(msgpack::object o) {
  if (o.type != msgpack::type::ARRAY || o.via.array.size != 2) {
    throw msgpack::type_error();
  }
  recommender_engine_->unpack(o.via.array.ptr[0]);

  msgpack::object labels = o.via.array.ptr[1];
  if (labels.type != msgpack::type::ARRAY) {
    throw msgpack::type_error();
  }
  for (size_t i = 0; i < labels.via.array.size; ++i) {
    std::string label;
    labels.via.array.ptr[i].convert(&label);
    {
      util::concurrent::scoped_lock lk(label_mutex_);
      labels_.insert(label);
    }
  }
}

framework::mixable* recommender_classifier::get_mixable() {
  return recommender_engine_->get_mixable();
}

void recommender_classifier::unlearn_id(const std::string& id) {
  recommender_engine_->clear_row(id);
}

}  // namespace classifier
}  // namespace core
}  // namespace jubatus
