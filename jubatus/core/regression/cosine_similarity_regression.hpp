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

#ifndef JUBATUS_CORE_REGRESSION_COSINE_SIMILARITY_REGRESSION_HPP_
#define JUBATUS_CORE_REGRESSION_COSINE_SIMILARITY_REGRESSION_HPP_

#include <string>
#include "inverted_index_regression.hpp"

namespace jubatus {
namespace core {
namespace regression {

class cosine_similarity_regression : public inverted_index_regression {
 public:
  explicit cosine_similarity_regression(const config& conf);
  float estimate(
      const common::sfv_t& fv) const;

  std::string name() const;
};

}  // namespace regression
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_REGRESSION_COSINE_SIMILARITY_REGRESSION_HPP_

