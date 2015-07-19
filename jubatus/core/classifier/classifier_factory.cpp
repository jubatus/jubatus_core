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

#include "classifier_factory.hpp"

#include <string>

#include "classifier.hpp"
#include "classifier_config.hpp"
#include "../common/exception.hpp"
#include "../storage/storage_base.hpp"
#include "../unlearner/unlearner_factory.hpp"
#include "../unlearner/unlearner_config.hpp"
#include "../nearest_neighbor/nearest_neighbor_factory.hpp"

using jubatus::util::lang::shared_ptr;
namespace jubatus {
namespace core {
namespace classifier {
namespace {

jubatus::util::lang::shared_ptr<unlearner::unlearner_base>
create_unlearner(const detail::unlearning_classifier_config& conf) {
  if (conf.unlearner_config_) {
    return unlearner::create_unlearner(conf.unlearner_config_);
  } else {
    return jubatus::util::lang::shared_ptr<unlearner::unlearner_base>();
  }
}
}  // namespace

shared_ptr<classifier_base> classifier_factory::create_classifier(
    const classifier_config& conf) {
  const classifier_config_base* conf_base = conf.conf_.get();
  jubatus::util::lang::shared_ptr<unlearner::unlearner_base> unlearner;
  shared_ptr<classifier_base> res;

  const detail::unlearning_classifier_config* uconf =
      dynamic_cast<const detail::unlearning_classifier_config*>(conf_base);
  const detail::nearest_neighbor_classifier_config* nconf =
      dynamic_cast<const detail::nearest_neighbor_classifier_config*>(
          conf_base);
  {  // unlearner
    if (uconf) {
      unlearner = create_unlearner(uconf->unlearner_config_);
    } else if (nconf) {
      unlearner = create_unlearner(nconf->unlearner_config_);
    } else {
      // no unlearner
    }
  }

  if (conf_base->method_ == "perceptron") {
    // perceptron doesn't have parameter
    JUBATUS_ASSERT(uconf != NULL);
    res.reset(new perceptron());
  } else if (conf_base->method_ == "PA" ||
      conf_base->method_ == "passive_aggressive") {
    // passive_aggressive doesn't have parameter
    JUBATUS_ASSERT(uconf != NULL);
    res.reset(new passive_aggressive());
  } else if (conf_base->method_ == "PA1" ||
      conf_base->method_ == "passive_aggressive_1") {
    JUBATUS_ASSERT(uconf != NULL);
    res.reset(new passive_aggressive_1(uconf->regularization_weight));
  } else if (conf_base->method_ == "PA2" ||
      conf_base->method_ == "passive_aggressive_2") {
    JUBATUS_ASSERT(uconf != NULL);
    res.reset(new passive_aggressive_2(uconf->regularization_weight));
  } else if (conf_base->method_ == "CW" ||
      conf_base->method_ == "confidence_weighted") {
    JUBATUS_ASSERT(uconf != NULL);
    res.reset(new confidence_weighted(uconf->regularization_weight));
  } else if (conf_base->method_ == "AROW" ||
      conf_base->method_ == "arow") {
    JUBATUS_ASSERT(uconf != NULL);
    res.reset(new arow(uconf->regularization_weight));
  } else if (conf_base->method_ == "NHERD" ||
      conf_base->method_ == "normal_herd") {
    JUBATUS_ASSERT(uconf != NULL);
    res.reset(new normal_herd(uconf->regularization_weight));
  } else if (conf_base->method_ == "NN" ||
      conf_base->method_ == "nearest_neighbor") {
    JUBATUS_ASSERT(nconf != NULL);
    shared_ptr<storage::column_table> table(new storage::column_table);
    /* // TODO
    shared_ptr<nearest_neighbor::nearest_neighbor_base>
        nearest_neighbor_engine(nearest_neighbor::create_nearest_neighbor(
            conf_base->method_, nconf->parameter, table, ""));
    */
    shared_ptr<nearest_neighbor::nearest_neighbor_base> nearest_neighbor_engine;
    res.reset(
        new nearest_neighbor_classifier(nearest_neighbor_engine,
                                        nconf->nearest_neighbor_num,
                                        nconf->local_sensitivity));
  } else {
    throw JUBATUS_EXCEPTION(
        common::unsupported_method("classifier(" + conf_base->method_ + ")"));
  }
  if (unlearner) {
    res->set_label_unlearner(unlearner);
  }
  return res;
}

}  // namespace classifier
}  // namespace core
}  // namespace jubatus
