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

#include "inverted_index_classifier.hpp"

#include <string>
#include <vector>
#include <map>
#include "jubatus/util/concurrent/lock.h"
#include "jubatus/util/concurrent/rwmutex.h"
#include "nearest_neighbor_classifier_util.hpp"

using jubatus::util::lang::shared_ptr;
using jubatus::util::concurrent::scoped_lock;

namespace jubatus {
namespace core {
namespace classifier {

inverted_index_classifier::inverted_index_classifier(
    size_t k,
    float alpha) :
  mixable_storage_(),
  labels_(core::storage::mixable_labels::model_ptr(
        new core::storage::labels())), k_(k), alpha_(alpha) {
  if (!(alpha >= 0)) {
    throw JUBATUS_EXCEPTION(common::invalid_parameter(
        "local_sensitivity should >= 0"));
  }
  if (!(k >= 1)) {
    throw JUBATUS_EXCEPTION(common::invalid_parameter(
        "nearest neighbor num >= 1"));
  }
  typedef storage::inverted_index_storage ii_storage;
  typedef storage::mixable_inverted_index_storage mii_storage;
  jubatus::util::lang::shared_ptr<ii_storage> p(new ii_storage);
  mixable_storage_.reset(new mii_storage(p));
}

void inverted_index_classifier::train(
    const common::sfv_t& fv,
    const std::string& label) {
  std::string id;
  {
    util::concurrent::scoped_lock lk(rand_mutex_);
    id = make_id_from_label(label, rand_);
  }
  {
    util::concurrent::scoped_wlock lk(storage_mutex_);
    storage::inverted_index_storage& inv = *mixable_storage_->get_model();
    for (size_t i = 0; i < fv.size(); ++i) {
      inv.set(fv[i].first, id, fv[i].second);
    }
  }
  set_label(label);
  labels_.get_model()->increment(label);
}

void inverted_index_classifier::set_label_unlearner(
    jubatus::util::lang::shared_ptr<unlearner::unlearner_base>
      label_unlearner) {
  // not implemented
}

bool inverted_index_classifier::delete_label(const std::string& label) {
  return false;
}

labels_t inverted_index_classifier::get_labels() const {
  return labels_.get_model()->get_labels();
}

bool inverted_index_classifier::set_label(const std::string& label) {
  return labels_.get_model()->add(label);
}

void inverted_index_classifier::get_status(
    std::map<std::string, std::string>& status) const {
}

void inverted_index_classifier::pack(framework::packer& pk) const {
  pk.pack_array(2);
  {
    util::concurrent::scoped_rlock lk(storage_mutex_);
    mixable_storage_->get_model()->pack(pk);
  }
  labels_.get_model()->pack(pk);
}

void inverted_index_classifier::unpack(msgpack::object o) {
  if (o.type != msgpack::type::ARRAY || o.via.array.size != 2) {
    throw msgpack::type_error();
  }
  {
    util::concurrent::scoped_wlock lk(storage_mutex_);
    mixable_storage_->get_model()->unpack(o.via.array.ptr[0]);
  }
  labels_.get_model()->unpack(o.via.array.ptr[1]);
}

void inverted_index_classifier::clear() {
  {
    util::concurrent::scoped_wlock lk(storage_mutex_);
    mixable_storage_->get_model()->clear();
  }
  labels_.get_model()->clear();
}

std::string inverted_index_classifier::name() const {
  return "inverted_index_classifier";
}

std::vector<framework::mixable*> inverted_index_classifier::get_mixables() {
  std::vector<framework::mixable*> mixables;
  mixables.push_back(mixable_storage_.get());
  mixables.push_back(&labels_);
  return mixables;
}

}  // namespace classifier
}  // namespace core
}  // namespalce jubatus
