// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2011 Preferred Infrastructure and Nippon Telegraph and Telephone Corporation.
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

#ifndef JUBATUS_CORE_WORDCOUNT_WORDCOUNT_BASE_HPP_
#define JUBATUS_CORE_WORDCOUNT_WORDCOUNT_BASE_HPP_

#include <stdint.h>

#include <cmath>
#include <map>
#include <string>
#include <vector>

#include "jubatus/util/data/unordered_map.h"
#include "jubatus/util/data/serialization.h"
#include "../framework/packer.hpp"
#include "../common/type.hpp"
#include "../framework/linear_function_mixer.hpp"
#include "../storage/storage_base.hpp"
#include "../unlearner/unlearner_base.hpp"

namespace jubatus {
namespace core {
namespace wordcount {

struct wordcount_config {
 public:
  explicit wordcount_config(int cap = 1024 * 1024)
      : capacity(cap) {
  }

  int capacity;

  template<typename Ar>
  void serialize(Ar& ar) {
    ar & JUBA_NAMED_MEMBER("capacity", capacity);
  }
};

class wordcount_base {
 public:
  virtual ~wordcount_base() {}

  virtual bool append(const std::string& bucket,
                      const common::sfv_t& words) = 0;

  virtual size_t count(const std::string& bucket,
                       const std::string& word) const = 0;

  virtual std::vector<std::pair<std::string, size_t> >
      get_ranking(const std::string& bucket, size_t size) const = 0;

  virtual void clear() = 0;

  virtual void pack(core::framework::packer& packer) const = 0;
  virtual void unpack(msgpack::object obj) = 0;

  virtual bool clear_bucket(const std::string& bucket) = 0;
};

}  // namespace wordcount
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_WORDCOUNT_WORDCOUNT_BASE_HPP_
