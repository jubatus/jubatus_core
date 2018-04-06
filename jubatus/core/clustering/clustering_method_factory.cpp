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

#include "clustering_method_factory.hpp"

#include <string>
#include "../common/exception.hpp"
#include "../common/jsonconfig.hpp"
#include "clustering_method.hpp"
#include "kmeans_clustering_method.hpp"
#include "dbscan_clustering_method.hpp"
#ifdef JUBATUS_USE_EIGEN
#include "gmm_clustering_method.hpp"
#endif

using jubatus::util::lang::shared_ptr;
using jubatus::core::common::jsonconfig::config;
using jubatus::core::common::jsonconfig::config_cast_check;

namespace jubatus {
namespace core {
namespace clustering {

shared_ptr<clustering_method> clustering_method_factory::create(
    const std::string& method,
    const std::string& distance,
    const config& config) {

  if (method == "kmeans") {
    kmeans_clustering_method::config conf =
      config_cast_check<kmeans_clustering_method::config>(config);
      return shared_ptr<clustering_method>(
        new kmeans_clustering_method(conf.k, conf.seed, distance));
  } else if (method == "dbscan") {
    dbscan_clustering_method::config conf =
      config_cast_check<dbscan_clustering_method::config>(config);
      return shared_ptr<clustering_method>(
        new dbscan_clustering_method(
            conf.eps,
            conf.min_core_point,
            distance));
#ifdef JUBATUS_USE_EIGEN
  } else if (method == "gmm") {
    gmm_clustering_method::config conf =
      config_cast_check<gmm_clustering_method::config>(config);
    return shared_ptr<clustering_method>(
        new gmm_clustering_method(conf.k, conf.seed));
#endif
  }
  throw JUBATUS_EXCEPTION(core::common::unsupported_method(method));
}

shared_ptr<clustering_method> clustering_method_factory::create(
    const std::string& method,
    const config& config) {

  if (method == "kmeans") {
    kmeans_clustering_method::config conf =
      config_cast_check<kmeans_clustering_method::config>(config);
      return shared_ptr<clustering_method>(
        new kmeans_clustering_method(conf.k, conf.seed));
  } else if (method == "dbscan") {
    dbscan_clustering_method::config conf =
      config_cast_check<dbscan_clustering_method::config>(config);
    return shared_ptr<clustering_method>(
      new dbscan_clustering_method(conf.eps, conf.min_core_point));

#ifdef JUBATUS_USE_EIGEN
  } else if (method == "gmm") {
    gmm_clustering_method::config conf =
      config_cast_check<gmm_clustering_method::config>(config);
    return shared_ptr<clustering_method>(
        new gmm_clustering_method(conf.k, conf.seed));
#endif
  }
  throw JUBATUS_EXCEPTION(core::common::unsupported_method(method));
}

}  // namespace clustering
}  // namespace core
}  // namespace jubatus
