// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2011,2012 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#ifndef JUBATUS_CORE_GRAPH_GRAPH_FACTORY_HPP_
#define JUBATUS_CORE_GRAPH_GRAPH_FACTORY_HPP_

#include <string>
#include "jubatus/util/lang/shared_ptr.h"

namespace jubatus {
namespace core {
namespace common {
namespace jsonconfig {

class config;

}  // namespace jsonconfig
}  // namespace common

namespace graph {

class graph_wo_index;

class graph_factory {
 public:
  static jubatus::util::lang::shared_ptr<graph_wo_index> create_graph(
    const std::string& name,
    const common::jsonconfig::config& param);
};

}  // namespace graph
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_GRAPH_GRAPH_FACTORY_HPP_
