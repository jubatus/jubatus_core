// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2014 Preferred Infrastructure and Nippon Telegraph and Telephone Corporation.
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

#include "select_by_weights.hpp"

#include <vector>
#include <numeric>

using jubatus::util::math::random::mtrand;

namespace jubatus {
namespace core {
namespace bandit {

int select_by_weights(
    const std::vector<double>& weights, mtrand& rand) {
  double total_weight = std::accumulate(weights.begin(), weights.end(), 0.0);
  double x = rand.next_double(total_weight);

  double w = 0;
  for (size_t i = 0; i < weights.size() - 1; ++i) {
    w += weights[i];
    if (x < w) {
      return i;
    }
  }
  return weights.size() - 1;
}

}  // namespace bandit
}  // namespace core
}  // namespace jubatus
