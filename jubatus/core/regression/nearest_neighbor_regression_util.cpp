
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

#include "nearest_neighbor_regression_util.hpp"
#include <string>

namespace jubatus {
namespace core {
namespace regression {

std::string make_id(
     jubatus::util::math::random::mtrand& rand) {
  const size_t n = 8;
  std::string result;
  result.reserve(1 + n);
  for (size_t i = 0; i < n; ++i) {
    int r = rand.next_int(26 * 2 + 10);
    if (r < 26) {
      result.push_back('a' + r);
    } else if (r < 26 * 2) {
      result.push_back('A' + (r - 26));
    } else {
      result.push_back('0' + (r - 26 * 2));
    }
  }
  return result;
}
}  // namespace regression
}  // namespace core
}  // namespace jubatus
