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

#ifndef JUBATUS_CORE_RECOMMENDER_INVERTED_INDEX_EUCLID_HPP_
#define JUBATUS_CORE_RECOMMENDER_INVERTED_INDEX_EUCLID_HPP_

#include <string>
#include <utility>
#include <vector>
#include "inverted_index.hpp"

namespace jubatus {
namespace core {
namespace recommender {

class inverted_index_euclid : public inverted_index {
 public:
  inverted_index_euclid();
  ~inverted_index_euclid();
  explicit inverted_index_euclid(
      jubatus::util::lang::shared_ptr<unlearner::unlearner_base> unlearner);

  void similar_row(
      const common::sfv_t& query,
      std::vector<std::pair<std::string, float> >& ids,
      size_t ret_num) const;
  void similar_row(
      const std::string& id,
      std::vector<std::pair<std::string, float> >& ids,
      size_t ret_num) const;
  void neighbor_row(
      const common::sfv_t& query,
      std::vector<std::pair<std::string, float> >& ids,
      size_t ret_num) const;
  void neighbor_row(
      const std::string& id,
      std::vector<std::pair<std::string, float> >& ids,
      size_t ret_num) const;

  std::string type() const;
};

}  // namespace recommender
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_RECOMMENDER_INVERTED_INDEX_EUCLID_HPP_
