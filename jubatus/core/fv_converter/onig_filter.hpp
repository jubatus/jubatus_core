// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2013 Preferred Infrastructure and Nippon Telegraph and Telephone Corporation.
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

#ifndef JUBATUS_CORE_FV_CONVERTER_ONIG_FILTER_HPP_
#define JUBATUS_CORE_FV_CONVERTER_ONIG_FILTER_HPP_

#include <oniguruma.h>
#include <string>
#include "string_filter.hpp"

namespace jubatus {
namespace core {
namespace fv_converter {

class regexp_filter : public string_filter {
 public:
  regexp_filter(const std::string& regexp, const std::string& replace);
  ~regexp_filter();

  void filter(const std::string& input, std::string& output) const;

 private:
  regexp_filter();

  void replace(
      const std::string& input,
      const OnigRegion* region,
      std::ostream& out) const;

  regex_t* reg_;
  std::string replace_;
};

}  // namespace fv_converter
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_FV_CONVERTER_ONIG_FILTER_HPP_
