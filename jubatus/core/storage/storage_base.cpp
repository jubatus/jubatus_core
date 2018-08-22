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

#include "storage_base.hpp"
#include <string>
#include "jubatus/util/text/json.h"

using std::string;

namespace jubatus {
namespace core {
namespace storage {

void storage_base::update(
    const string& feature,
    const string& inc_class,
    const string& dec_class, const val1_t& w) {
  feature_val1_t row;
  get(feature, row);
  double inc_class_val = w;
  double dec_class_val = -w;
  for (size_t i = 0; i < row.size(); ++i) {
    const string& label = row[i].first;
    if (label == inc_class) {
      inc_class_val += row[i].second;
    } else if (label == dec_class) {
      dec_class_val += row[i].second;
    }
  }
  set(feature, inc_class, inc_class_val);
  set(feature, dec_class, dec_class_val);
}

void storage_base::bulk_update(
    const common::sfv_t& sfv,
    double step_width,
    const std::string& inc_class,
    const std::string& dec_class) {
  for (common::sfv_t::const_iterator it = sfv.begin(); it != sfv.end(); ++it) {
    const string& feature = it->first;
    double val = it->second;
    if (dec_class != "") {
      update(feature, inc_class, dec_class, step_width * val);
    } else {
      feature_val1_t ret;
      get(feature, ret);
      double pos_val = 0.0;
      for (size_t i = 0; i < ret.size(); ++i) {
        if (ret[i].first == inc_class) {
          pos_val = ret[i].second;
          break;
        }
      }
      set(feature, inc_class, pos_val + step_width * val);
    }
  }
}

void storage_base::get_diff(diff_t& v) const {
  v.diff.clear();
}

bool storage_base::set_average_and_clear_diff(const diff_t&) {
  return true;
}

}  // namespace storage
}  // namespace core
}  // namespace jubatus
