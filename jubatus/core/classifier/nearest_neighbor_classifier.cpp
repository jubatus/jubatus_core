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

#include "nearest_neighbor_classifier.hpp"

#include <string>
#include <vector>
#include <map>
#include <utility>

using jubatus::util::lang::shared_ptr;

namespace jubatus {
namespace core {
namespace classifier {

namespace {
std::string make_id_from_label(const std::string& label,
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

class nearest_neighbor_classifier::unlearning_callback {
 public:
  explicit unlearning_callback(nearest_neighbor_classifier* classifier)
      : classifier_(classifier) {
  }

  void operator()(const std::string& id) {
    classifier_->unlearn_id(id);
  }

 private:
  nearest_neighbor_classifier* classifier_;
};

nearest_neighbor_classifier::nearest_neighbor_classifier(
    shared_ptr<nearest_neighbor::nearest_neighbor_base> engine,
    size_t k)
    : nearest_neighbor_engine_(engine), k_(k) {
}

void nearest_neighbor_classifier::train(
    const common::sfv_t& fv, const std::string& label) {
  std::string id = make_id_from_label(label, rand_);
  if (unlearner_) {
    if (!unlearner_->touch(id)) {
      throw JUBATUS_EXCEPTION(common::exception::runtime_error(
          "no more space available to add new ID: " + id));
    }
  }
  nearest_neighbor_engine_->set_row(id, fv);
}

void nearest_neighbor_classifier::set_label_unlearner(
    shared_ptr<unlearner::unlearner_base> label_unlearner) {
  label_unlearner->set_callback(unlearning_callback(this));
  unlearner_ = label_unlearner;
}

void nearest_neighbor_classifier::classify_with_scores(
    const common::sfv_t& fv, classify_result& scores) const {
  std::vector<std::pair<std::string, float> > ids;
  nearest_neighbor_engine_->neighbor_row(fv, ids, k_);

  std::map<std::string, float> m;
  for (size_t i = 0; i < ids.size(); ++i) {
    std::string label = get_label_from_id(ids[i].first);
    m[label] += std::exp(-ids[i].second);
  }

  scores.clear();
  for (std::map<std::string, float>::const_iterator iter = m.begin();
       iter != m.end(); ++iter) {
    classify_result_elem elem(iter->first, iter->second);
    scores.push_back(elem);
  }
}

bool nearest_neighbor_classifier::delete_label(const std::string& label) {
  // unimplemented
  throw JUBATUS_EXCEPTION(common::unsupported_method(__func__));
}

void nearest_neighbor_classifier::clear() {
  nearest_neighbor_engine_->clear();
  if (unlearner_) {
    unlearner_->clear();
  }
}

std::vector<std::string> nearest_neighbor_classifier::get_labels() const {
  // unimplemented
  throw JUBATUS_EXCEPTION(common::unsupported_method(__func__));
}

bool nearest_neighbor_classifier::set_label(const std::string& label) {
  // unimplemented
  throw JUBATUS_EXCEPTION(common::unsupported_method(__func__));
}

std::string nearest_neighbor_classifier::name() const {
  return "nearest_neighbor_classifier:" + nearest_neighbor_engine_->type();
}

void nearest_neighbor_classifier::get_status(
    std::map<std::string, std::string>& status) const {
  // unimplemented
}

void nearest_neighbor_classifier::pack(framework::packer& pk) const {
  nearest_neighbor_engine_->pack(pk);
}

void nearest_neighbor_classifier::unpack(msgpack::object o) {
  nearest_neighbor_engine_->unpack(o);
}

framework::mixable* nearest_neighbor_classifier::get_mixable() {
  return nearest_neighbor_engine_->get_mixable();
}

void nearest_neighbor_classifier::unlearn_id(const std::string& id) {
  nearest_neighbor_engine_->get_table()->delete_row(id);
}

}  // namespace classifier
}  // namespace core
}  // namespace jubatus
