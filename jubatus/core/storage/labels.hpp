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

#ifndef JUBATUS_CORE_STORAGE_LABELS_HPP_
#define JUBATUS_CORE_STORAGE_LABELS_HPP_

#include <stdint.h>
#include <algorithm>
#include <string>
#include <msgpack.hpp>

#include "jubatus/util/concurrent/rwmutex.h"
#include "jubatus/util/data/unordered_map.h"
#include "../common/unordered_map.hpp"
#include "../common/version.hpp"
#include "../framework/mixable_helper.hpp"

using jubatus::util::data::unordered_map;

namespace jubatus {
namespace core {
namespace storage {

class labels {
 public:
  typedef unordered_map<std::string, uint64_t> data_t;

  labels();
  ~labels();

  bool add(const std::string& label);
  unordered_map<std::string, uint64_t> get_labels() const;

  void increment(const std::string& label);
  void decrement(const std::string& label);
  bool erase(const std::string& label);

  void clear();

  void swap(data_t& labels);

  void get_diff(data_t& diff) const;
  bool put_diff(const data_t& mixed_diff);
  void mix(const data_t& lhs, data_t& rhs) const;

  version get_version() const {
    return version_;
  }

  std::string name() const {
    return std::string("labels");
  }

  void pack(framework::packer& packer) const;
  void unpack(msgpack::object o);

  MSGPACK_DEFINE(master_, diff_, version_);

 private:
  data_t master_;
  data_t diff_;

  version version_;

  mutable jubatus::util::concurrent::rw_mutex mutex_;
};

typedef framework::linear_mixable_helper<labels, labels::data_t> mixable_labels;

}  // namespace storage
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_STORAGE_LABELS_HPP_
