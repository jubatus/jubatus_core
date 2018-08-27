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

#include "linear_classifier.hpp"

#include <assert.h>
#include <float.h>

#include <algorithm>
#include <map>
#include <queue>
#include <string>
#include <vector>
#include "jubatus/util/concurrent/lock.h"
#include "jubatus/util/lang/bind.h"

#include "../common/exception.hpp"
#include "classifier_util.hpp"

using std::string;
using std::vector;
using jubatus::core::storage::map_feature_val1_t;
using jubatus::core::storage::feature_val2_t;

namespace jubatus {
namespace core {
namespace classifier {

linear_classifier::linear_classifier(storage_ptr storage)
  : storage_(storage), mixable_storage_(storage_),
    labels_(core::storage::mixable_labels::model_ptr(
        new core::storage::labels())) {
}

linear_classifier::~linear_classifier() {
}

namespace {
// This function is a workaround for libc++.
// libc++'s std::function<void (<any types>)> does not accept
// functions which returns other than void.
void delete_label_wrapper(linear_classifier* cb, const std::string& label) {
    cb->unlearn_label(label);
}
}

void linear_classifier::set_label_unlearner(
    jubatus::util::lang::shared_ptr<unlearner::unlearner_base>
        label_unlearner) {
  label_unlearner->set_callback(
      jubatus::util::lang::bind(
          delete_label_wrapper, this, jubatus::util::lang::_1));
  mixable_storage_.set_label_unlearner(label_unlearner);
  unlearner_ = label_unlearner;
}

void linear_classifier::classify_with_scores(
    const common::sfv_t& sfv,
    classify_result& scores) const {
  scores.clear();

  map_feature_val1_t ret;
  storage_->inp(sfv, ret);
  for (map_feature_val1_t::const_iterator it = ret.begin(); it != ret.end();
      ++it) {
    scores.push_back(classify_result_elem(it->first, it->second));
  }
}

string linear_classifier::classify(const common::sfv_t& fv) const {
  classify_result result;
  classify_with_scores(fv, result);
  double max_score = -DBL_MAX;
  string max_class;
  for (vector<classify_result_elem>::const_iterator it = result.begin();
      it != result.end(); ++it) {
    if (it == result.begin() || it->score > max_score) {
      max_score = it->score;
      max_class = it->label;
    }
  }
  return max_class;
}

void linear_classifier::clear() {
  storage_->clear();
  labels_.get_model()->clear();
  if (unlearner_) {
    util::concurrent::scoped_lock unlearner_lk(unlearner_mutex_);
    unlearner_->clear();
  }
}

labels_t linear_classifier::get_labels() const {
  return labels_.get_model()->get_labels();
}

bool linear_classifier::set_label(const string& label) {
  check_touchable(label);

  bool result = storage_->set_label(label);
  result = labels_.get_model()->add(label) && result;
  if (unlearner_ && result) {
    util::concurrent::scoped_lock unlearner_lk(unlearner_mutex_);
    result = unlearner_->touch(label);
  }

  return result;
}

void linear_classifier::get_status(std::map<string, string>& status) const {
  storage_->get_status(status);
  status["storage"] = storage_->type();

  if (unlearner_) {
    util::concurrent::scoped_lock unlearner_lk(unlearner_mutex_);
    unlearner_->get_status(status);
  }
}

void linear_classifier::update_weight(
    const common::sfv_t& sfv,
    double step_width,
    const string& pos_label,
    const string& neg_label) {
  storage_->bulk_update(sfv, step_width, pos_label, neg_label);
}

string linear_classifier::get_largest_incorrect_label(
    const common::sfv_t& fv,
    const string& label,
    classify_result& scores) const {
  classify_with_scores(fv, scores);
  double max_score = -DBL_MAX;
  string max_class;
  for (vector<classify_result_elem>::const_iterator it = scores.begin();
      it != scores.end(); ++it) {
    if (it->label == label) {
      continue;
    }
    if (it->score > max_score || it == scores.begin()) {
      max_score = it->score;
      max_class = it->label;
    }
  }
  return max_class;
}

double linear_classifier::calc_margin(
    const common::sfv_t& fv,
    const string& label,
    string& incorrect_label) const {
  classify_result scores;
  incorrect_label = get_largest_incorrect_label(fv, label, scores);
  double correct_score = 0.0;
  double incorrect_score = 0.0;
  for (vector<classify_result_elem>::const_iterator it = scores.begin();
      it != scores.end(); ++it) {
    if (it->label == label) {
      correct_score = it->score;
    } else if (it->label == incorrect_label) {
      incorrect_score = it->score;
    }
  }
  return incorrect_score - correct_score;
}

double linear_classifier::calc_margin_and_variance(
    const common::sfv_t& sfv,
    const string& label,
    string& incorrect_label,
    double& var) const {
  double margin = calc_margin(sfv, label, incorrect_label);
  var = 0.0;

  util::concurrent::scoped_rlock lk(storage_->get_lock());
  for (size_t i = 0; i < sfv.size(); ++i) {
    const string& feature = sfv[i].first;
    const double val = sfv[i].second;
    feature_val2_t weight_covars;
    storage_->get2_nolock(feature, weight_covars);
    double label_covar = 1.0;
    double incorrect_label_covar = 1.0;
    for (size_t j = 0; j < weight_covars.size(); ++j) {
      if (weight_covars[j].first == label) {
        label_covar = weight_covars[j].second.v2;
      } else if (weight_covars[j].first == incorrect_label) {
        incorrect_label_covar = weight_covars[j].second.v2;
      }
    }
    var += (label_covar + incorrect_label_covar) * val * val;
  }
  return margin;
}

double linear_classifier::squared_norm(const common::sfv_t& fv) {
  double ret = 0.0;
  for (size_t i = 0; i < fv.size(); ++i) {
    ret += fv[i].second * fv[i].second;
  }
  return ret;
}

void linear_classifier::pack(framework::packer& pk) const {
  if (unlearner_) {
    pk.pack_array(3);

    util::concurrent::scoped_lock unlearner_lk(unlearner_mutex_);
    unlearner_->pack(pk);
  } else {
    pk.pack_array(2);
  }

  storage_->pack(pk);
  labels_.get_model()->pack(pk);
}
void linear_classifier::unpack(msgpack::object o) {
  if (o.type != msgpack::type::ARRAY) {
    throw msgpack::type_error();
  }

  size_t i = 0;

  if (unlearner_) {
    if (o.via.array.size != 3) {
      throw msgpack::type_error();
    }

    util::concurrent::scoped_lock unlearner_lk(unlearner_mutex_);
    unlearner_->unpack(o.via.array.ptr[i]);
    ++i;
  } else if (o.via.array.size != 2) {
    throw msgpack::type_error();
  }

  storage_->unpack(o.via.array.ptr[i]);
  labels_.get_model()->unpack(o.via.array.ptr[i+1]);
}

std::vector<framework::mixable*> linear_classifier::get_mixables() {
  std::vector<framework::mixable*> mixables;
  mixables.push_back(&mixable_storage_);
  mixables.push_back(&labels_);
  return mixables;
}

void linear_classifier::touch(const std::string& label) {
  check_touchable(label);

  if (unlearner_) {
    util::concurrent::scoped_lock unlearner_lk(unlearner_mutex_);
    unlearner_->touch(label);
  }
}

void linear_classifier::check_touchable(const std::string& label) {
  if (unlearner_) {
    util::concurrent::scoped_lock unlearner_lk(unlearner_mutex_);
    if (!unlearner_->can_touch(label)) {
      throw JUBATUS_EXCEPTION(common::exception::runtime_error(
          "cannot add new label as number of sticky labels reached "
          "the maximum size of unlearner: " + label));
    }
  }
}

bool linear_classifier::delete_label(const std::string& label) {
  // Remove the label from the model.
  bool result = storage_->delete_label(label);
  result = labels_.get_model()->erase(label) && result;

  if (unlearner_ && result) {
    util::concurrent::scoped_lock unlearner_lk(unlearner_mutex_);
    // Notify unlearner that the label was removed.
    result = unlearner_->remove(label);
  }

  return result;
}

/**
 * Callback function to delete the label via unlearner.
 */
bool linear_classifier::unlearn_label(const std::string& label) {
  // this method must be called via touch() function.
  // touch() must be done with holding lock
  // so this function must not get lock
  bool result = storage_->delete_label_nolock(label);
  result = labels_.get_model()->erase(label) && result;
  return result;
}

}  // namespace classifier
}  // namespace core
}  // namespace jubatus
