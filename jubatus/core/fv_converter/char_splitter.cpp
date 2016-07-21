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

#include "char_splitter.hpp"

#include <string>
#include <utility>
#include <vector>

#include "jubatus/util/data/string/ustring.h"

using jubatus::util::data::string::string_to_ustring;
using jubatus::util::data::string::ustring_to_string;

namespace jubatus {
namespace core {
namespace fv_converter {

void char_splitter::split(
    const std::string& string,
    std::vector<std::pair<size_t, size_t> >& ret_boundaries) const {
  std::vector<std::pair<size_t, size_t> > bounds;
  const jubatus::util::data::string::ustring target = string_to_ustring(string);

  size_t last = 0;
  while (true) {
    size_t begin = target.find_first_not_of(separator_, last);
    if (begin == std::string::npos) {
      break;
    }

    size_t begin_bytes = ustring_to_string(target.substr(0, begin)).size();
    size_t end = target.find_first_of(separator_, begin);
    if (end == std::string::npos) {
      size_t len_bytes = ustring_to_string(target.substr(begin)).size();
      bounds.push_back(std::make_pair(begin_bytes, len_bytes));
      break;
    } else {
      size_t len = end - begin;
      size_t len_bytes = ustring_to_string(target.substr(begin, len)).size();
      bounds.push_back(std::make_pair(begin_bytes, len_bytes));
      last = end;
    }
  }

  bounds.swap(ret_boundaries);
}

}  // namespace fv_converter
}  // namespace core
}  // namespace jubatus
