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

#ifndef JUBATUS_CORE_CLUSTERING_CLUSTERING_HPP_
#define JUBATUS_CORE_CLUSTERING_CLUSTERING_HPP_

#include <stdint.h>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>
#include "jubatus/util/concurrent/rwmutex.h"
#include "jubatus/util/data/unordered_map.h"
#include "jubatus/util/lang/shared_ptr.h"
#include "../common/exception.hpp"
#include "../common/type.hpp"
#include "../framework/mixable.hpp"
#include "clustering_method.hpp"
#include "clustering_config.hpp"
#include "storage.hpp"
#include "types.hpp"

namespace jubatus {
namespace core {
namespace clustering {

class clustering_error : public common::exception::runtime_error {
 public:
  explicit clustering_error(const std::string& msg)
      : runtime_error(msg) {
  }
};

class not_performed : public clustering_error {
 public:
  explicit not_performed()
    : clustering_error("clustering is not performed yet") {
  }
};

class clustering {
 public:
  clustering(
      const std::string& name,
      const std::string& method,
      const clustering_config& cfg);
  virtual ~clustering();

  bool push(const std::vector<weighted_point>& points);
  wplist get_coreset() const;
  std::vector<wplist> get_core_members() const;
  wplist get_nearest_members(const common::sfv_t& point) const;
  std::vector<common::sfv_t> get_k_center() const;
  common::sfv_t get_nearest_center(const common::sfv_t& point) const;

  size_t get_revision() const;

  void set_storage(jubatus::util::lang::shared_ptr<storage> storage);
  jubatus::util::lang::shared_ptr<storage> get_storage();

  void set_clustering_method(
      jubatus::util::lang::shared_ptr<clustering_method> clustering_method);

  framework::mixable* get_mixable() const;
  std::string type() const;

  void pack(framework::packer& pk) const;
  void unpack(msgpack::object o);
  void clear();

  // for test only
  void do_clustering();

 private:
  void init();
  void update_clusters(const wplist& points, bool batch);
  wplist get_coreset_mine() const;

  clustering_config config_;
  std::string name_;
  std::string method_;

  jubatus::util::lang::shared_ptr<clustering_method> clustering_method_;
  jubatus::util::lang::shared_ptr<mixable_storage> storage_;
};

}  // namespace clustering
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_CLUSTERING_CLUSTERING_HPP_
