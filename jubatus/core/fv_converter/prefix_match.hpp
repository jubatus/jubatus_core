// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2011 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#ifndef JUBATUS_CORE_FV_CONVERTER_PREFIX_MATCH_HPP_
#define JUBATUS_CORE_FV_CONVERTER_PREFIX_MATCH_HPP_

#include <string>
#include "jubatus/util/data/string/utility.h"
#include "key_matcher.hpp"

namespace jubatus {
namespace core {
namespace fv_converter {

class prefix_match : public key_matcher {
 public:
  explicit prefix_match(const std::string& prefix)
      : prefix_(prefix) {
  }

  ~prefix_match() {
  }

  bool match(const std::string& key) {
    return jubatus::util::data::string::starts_with(key, prefix_);
  }

 private:
  const std::string prefix_;
};

}  // namespace fv_converter
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_FV_CONVERTER_PREFIX_MATCH_HPP_
