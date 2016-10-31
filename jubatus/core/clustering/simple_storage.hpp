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

#ifndef JUBATUS_CORE_CLUSTERING_SIMPLE_STORAGE_HPP_
#define JUBATUS_CORE_CLUSTERING_SIMPLE_STORAGE_HPP_

#include <string>
#include "storage.hpp"

namespace jubatus {
namespace core {
namespace clustering {

class simple_storage : public storage {
 public:
  struct config {
    config()
      : bucket_size(10000) {
    }
    int bucket_size;
    MSGPACK_DEFINE(bucket_size);
    template<typename Ar>
    void serialize(Ar& ar) {
      ar & JUBA_MEMBER(bucket_size);
    }
  };

  simple_storage(const std::string& name, const int bucket_size);

  void add(const weighted_point& point);
  wplist get_mine() const;

 private:
  int bucket_size_;
  wplist mine_;
  void pack_impl_(framework::packer& packer) const;
  void unpack_impl_(msgpack::object o);
  void clear_impl_();
};

}  // namespace clustering
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_CLUSTERING_SIMPLE_STORAGE_HPP_
