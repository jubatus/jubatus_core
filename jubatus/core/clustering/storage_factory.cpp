// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2013 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#include "storage_factory.hpp"

#include <string>
#include "../common/exception.hpp"
#include "compressive_storage.hpp"
#include "gmm_compressor.hpp"
#include "kmeans_compressor.hpp"
#include "simple_storage.hpp"
#include "../common/jsonconfig.hpp"
#include "storage.hpp"

using jubatus::core::common::jsonconfig::config;
using jubatus::core::common::jsonconfig::config_cast_check;

namespace jubatus {
namespace core {
namespace clustering {

jubatus::util::lang::shared_ptr<storage> storage_factory::create(
    const std::string& name,
    const std::string& method,
    const std::string& compressor_method,
    const config& compressor_param) {
  typedef jubatus::util::lang::shared_ptr<storage> ptr;
  ptr ret;

  if (method == "kmeans") {
    if (compressor_method == "simple") {
      simple_storage::config conf =
        config_cast_check<simple_storage::config>(compressor_param);
      simple_storage *s = new simple_storage(name, conf.bucket_size);
      ret.reset(s);
    } else if (compressor_method == "compressive") {
      compressive_storage::config conf =
        config_cast_check<compressive_storage::config>(compressor_param);

      compressive_storage *s = new compressive_storage(
                                       name,
                                       conf.bucket_size,
                                       conf.bucket_length,
                                       conf.compressed_bucket_size,
                                       conf.bicriteria_base_size,
                                       conf.forgetting_factor,
                                       conf.forgetting_threshold);
      s->set_compressor(jubatus::util::lang::shared_ptr<compressor::compressor>(
              new compressor::kmeans_compressor(conf.seed)));
      ret.reset(s);
    } else {
      throw JUBATUS_EXCEPTION(
          common::unsupported_method(compressor_method));
    }
  } else if (method == "dbscan") {
    if (compressor_method == "simple") {
      simple_storage::config conf =
         config_cast_check<simple_storage::config>(compressor_param);
      simple_storage *s = new simple_storage(name, conf.bucket_size);
      ret.reset(s);
    } else {
      throw JUBATUS_EXCEPTION(
          common::unsupported_method(compressor_method));
    }
#ifdef JUBATUS_USE_EIGEN
  } else if (method == "gmm") {
    if (compressor_method == "simple") {
      simple_storage::config conf =
        config_cast_check<simple_storage::config>(compressor_param);
      simple_storage *s = new simple_storage(name, conf.bucket_size);
      ret.reset(s);
    } else if (compressor_method == "compressive") {
      compressive_storage::config conf =
        config_cast_check<compressive_storage::config>(compressor_param);

      compressive_storage *s = new compressive_storage(
                                        name,
                                        conf.bucket_size,
                                        conf.bucket_length,
                                        conf.compressed_bucket_size,
                                        conf.bicriteria_base_size,
                                        conf.forgetting_factor,
                                        conf.forgetting_threshold);
      s->set_compressor(jubatus::util::lang::shared_ptr<compressor::compressor>(
             new compressor::gmm_compressor(conf.seed)));
      ret.reset(s);
#endif
    } else {
      throw JUBATUS_EXCEPTION(
          common::unsupported_method(compressor_method));
    }
  } else {
    throw JUBATUS_EXCEPTION(
      common::unsupported_method(method));
  }
  return ret;
}

}  // namespace clustering
}  // namespace core
}  // namespace jubatus
