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

#ifndef JUBATUS_CORE_CLASSIFIER_CLASSIFIER_TYPE_HPP_
#define JUBATUS_CORE_CLASSIFIER_CLASSIFIER_TYPE_HPP_

#include <stdint.h>

#include <vector>
#include <string>

#include "jubatus/util/data/unordered_map.h"
#include "classifier_config.hpp"

namespace jubatus {
namespace core {
namespace classifier {

// TODO(unknown): namespace should be classifier
struct classify_result_elem {
  classify_result_elem(const std::string& label, double score)
      : label(label),
        score(score) {
  };
  std::string label;
  double score;
};

typedef std::vector<classify_result_elem> classify_result;
typedef jubatus::util::data::unordered_map<std::string, uint64_t> labels_t;

}  // namespace classifier
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_CLASSIFIER_CLASSIFIER_TYPE_HPP_
