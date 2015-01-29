// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2015 Preferred Infrastructure and Nippon Telegraph and Telephone Corporation.
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

#ifndef JUBATUS_CORE_WORDCOUNT_SPACE_SAVING_BASE_HPP_
#define JUBATUS_CORE_WORDCOUNT_SPACE_SAVING_BASE_HPP_

#include <stdint.h>

#include <cmath>
#include <map>
#include <string>
#include <vector>

#include "jubatus/util/data/unordered_map.h"
#include "jubatus/util/lang/shared_ptr.h"
#include "../common/type.hpp"
#include "../framework/linear_function_mixer.hpp"
#include "wordcount_base.hpp"

namespace jubatus {
namespace core {
namespace wordcount {

class space_saving : public wordcount_base {
 public:
  explicit space_saving(const wordcount_config& conf);
  virtual ~space_saving() {}

  bool append(const std::string& bucket,
              const common::sfv_t& datum);

  size_t count(const std::string& bucket, const std::string& word) const;

  std::vector<std::pair<std::string, size_t> >
      get_ranking(const std::string& bucket, size_t size) const;

  void clear();

  bool clear_bucket(const std::string& bucket);

  void pack(core::framework::packer& packer) const;
  void unpack(msgpack::object obj);

private:
  class space_saving_impl;
  util::lang::shared_ptr<space_saving_impl> impl_;
};

}  // namespace wordcount
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_SPACE_SAVING_SPACE_SAVING_BASE_HPP_
