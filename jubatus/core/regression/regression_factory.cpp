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

#include "regression_factory.hpp"

#include <stdexcept>
#include <string>

#include "regression.hpp"
#include "../common/exception.hpp"
#include "../common/jsonconfig.hpp"
#include "../storage/storage_base.hpp"
#include "../unlearner/unlearner_factory.hpp"
#include "../storage/column_table.hpp"
#include "../nearest_neighbor/nearest_neighbor_factory.hpp"

using jubatus::core::common::jsonconfig::config_cast_check;
using jubatus::util::lang::shared_ptr;
using jubatus::core::common::jsonconfig::config;

namespace jubatus {
namespace core {
namespace regression {

jubatus::util::lang::shared_ptr<unlearner::unlearner_base>
create_unlearner(const nearest_neighbor_regression::unlearner_config& conf) {
  if (conf.unlearner) {
    if (!conf.unlearner_parameter) {
      throw JUBATUS_EXCEPTION(common::exception::runtime_error(
          "Unlearner is set but unlearner_parameter is not found"));
    }
    return core::unlearner::create_unlearner(
        *conf.unlearner, *conf.unlearner_parameter);
  } else {
    return jubatus::util::lang::shared_ptr<unlearner::unlearner_base>();
  }
}

shared_ptr<regression_base> regression_factory::create_regression(
    const std::string& name,
    const common::jsonconfig::config& param,
    shared_ptr<storage::storage_base> storage) {
  if (name == "PA" || name == "passive_aggressive") {
    return shared_ptr<regression_base>(new regression::passive_aggressive(
      config_cast_check<regression::passive_aggressive::config>(param),
      storage));
  } else if (name == "PA1" || name == "passive_aggressive_1") {
    return shared_ptr<regression_base>(new regression::passive_aggressive_1(
      config_cast_check<regression::passive_aggressive_1::config>(param),
      storage));
  } else if (name == "PA2" || name == "passive_aggressive_2") {
    return shared_ptr<regression_base>(new regression::passive_aggressive_2(
      config_cast_check<regression::passive_aggressive_2::config>(param),
      storage));
  } else if (name == "perceptron") {
    return shared_ptr<regression_base>(new regression::perceptron(
      config_cast_check<regression::perceptron::config>(param),
      storage));
  } else if (name == "CW" || name == "confidence_weighted") {
    return shared_ptr<regression_base>(new regression::confidence_weighted(
      config_cast_check<regression::confidence_weighted::config>(param),
      storage));
  } else if (name == "AROW") {
    return shared_ptr<regression_base>(new regression::arow(
      config_cast_check<regression::arow::config>(param),
      storage));
  } else if (name == "NHERD" || name == "normal_herd") {
    return shared_ptr<regression_base>(new regression::normal_herd(
      config_cast_check<regression::normal_herd::config>(param),
      storage));
  } else if (name == "NN" || name == "nearest_neighbor") {
    if (param.type() == jubatus::util::text::json::json::Null) {
      throw JUBATUS_EXCEPTION(
          common::config_exception() << common::exception::error_message(
              "parameter block is not specified in config"));
    }
    regression::nearest_neighbor_regression::config conf
      = config_cast_check<regression::nearest_neighbor_regression::config>(
          param);
    jubatus::util::lang::shared_ptr<unlearner::unlearner_base> unlearner;
    unlearner = create_unlearner(conf);
    shared_ptr<storage::column_table> table(new storage::column_table);
    shared_ptr<nearest_neighbor::nearest_neighbor_base>
        nearest_neighbor_engine(nearest_neighbor::create_nearest_neighbor(
            conf.method, conf.parameter, table, ""));
    shared_ptr<regression_base> res(new regression::nearest_neighbor_regression(
                                        nearest_neighbor_engine,
                                        conf));
    if (unlearner) {
      res->set_unlearner(unlearner);
    }
    return res;
  } else if (name == "cosine") {
    if (param.type() == jubatus::util::text::json::json::Null) {
      throw JUBATUS_EXCEPTION(
              common::config_exception() << common::exception::error_message(
              "parameter block is not specified in config"));
    }
    regression::inverted_index_regression::config conf
      = config_cast_check<regression::inverted_index_regression::config>(param);
    return shared_ptr<regression::cosine_similarity_regression> (
      new regression::cosine_similarity_regression(conf));
  } else if (name == "euclidean") {
    if (param.type() == jubatus::util::text::json::json::Null) {
      throw JUBATUS_EXCEPTION(
          common::config_exception() << common::exception::error_message(
              "parameter block is not specified in config"));
    }
    regression::inverted_index_regression::config conf
      = config_cast_check<regression::inverted_index_regression::config>(param);
    return shared_ptr<regression::euclidean_distance_regression> (
            new regression::euclidean_distance_regression(conf));
  } else {
    throw JUBATUS_EXCEPTION(common::unsupported_method(name));
  }
}

}  // namespace regression
}  // namespace core
}  // namespace jubatus
