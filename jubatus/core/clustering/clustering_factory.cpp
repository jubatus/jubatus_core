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

#include "clustering_factory.hpp"

#include <string>

#include "clustering.hpp"
#include "clustering_method_factory.hpp"
#include "storage_factory.hpp"
#include "storage.hpp"
#include "../common/jsonconfig.hpp"

using jubatus::core::common::jsonconfig::config;
using jubatus::core::common::jsonconfig::config_cast_check;
using jubatus::util::lang::shared_ptr;

namespace jubatus {
namespace core {
namespace clustering {

  shared_ptr<clustering> clustering_factory::create(
    const std::string& name,
    const std::string& method,
    const std::string& compressor,
    const config& method_param,
    const config& compressor_param) {
  shared_ptr<clustering_method>
    clustering_method(clustering_method_factory::create(method, method_param));
  shared_ptr<storage> storage(storage_factory::create(
                                  name,
                                  method,
                                  compressor,
                                  compressor_param));
  shared_ptr<clustering> cl(new clustering(clustering_method, storage));
  return cl;
}

}  // namespace clustering
}  // namespace core
}  // namespace jubatus
