// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2013 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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


#include "onig_filter.hpp"
#include <string>
#include <sstream>
#include "exception.hpp"

namespace jubatus {
namespace core {
namespace fv_converter {

regexp_filter::regexp_filter(const std::string& regexp,
    const std::string& replace)
    : reg_(NULL),
      replace_(replace) {
  const UChar* pattern = reinterpret_cast<const UChar*>(regexp.c_str());
  if (ONIG_NORMAL != onig_new(&reg_, pattern, pattern + regexp.size(),
        ONIG_OPTION_DEFAULT, ONIG_ENCODING_UTF8, ONIG_SYNTAX_PERL, NULL)) {
    throw JUBATUS_EXCEPTION(converter_exception(
        "invalid regular expression: " + regexp));
  }

  int num_captures = onig_number_of_captures(reg_);
  for (std::size_t i = 0; i < replace.size(); ++i) {
    char c = replace[i];
    if (c == '\\') {
      if (i == replace.size() - 1) {
        throw JUBATUS_EXCEPTION(converter_exception(
            "invalid replacement expression. 0-9 or \\ are required after \\"));
      }
      ++i;
      c = replace[i];
      if (c == '\\') {
      } else if ('0' <= c && c <= '9') {
        int group = c - '0';
        if (group > num_captures) {
          throw JUBATUS_EXCEPTION(converter_exception(
              "invalid number of capture group"));
        }
      } else {
        throw JUBATUS_EXCEPTION(converter_exception(
            "invalid replacement expression. 0-9 or \\ are required after \\"));
      }
    }
  }
}

regexp_filter::~regexp_filter() {
  if (reg_) {
    onig_free(reg_);
  }
}

void regexp_filter::replace(
    const std::string& input,
    const OnigRegion* region,
    std::ostream& out) const {
  for (std::size_t i = 0; i < replace_.size(); ++i) {
    char c = replace_[i];
    if (c == '\\') {
      ++i;
      if (i > replace_.size() - 1) {
        // This exception must not be called, because replace string is checked
        // in the constructor
        throw JUBATUS_EXCEPTION(converter_exception(
            "invalid replacement expression. 0-9 or \\ are required after \\"));
      }
      c = replace_[i];
      if (c == '\\') {
        out << '\\';
      } else if ('0' <= c && c <= '9') {
        int group = c - '0';
        std::size_t len = region->end[group] - region->beg[group];
        out << input.substr(region->beg[group], len);
      } else {
        // This exception must not be called, because replace string is checked
        // in the constructor
        throw JUBATUS_EXCEPTION(converter_exception(
            "invalid replacement expression. 0-9 or \\ are required after \\"));
      }
    } else {
      out << c;
    }
  }
}

void regexp_filter::filter(
    const std::string& input, std::string& output) const {
  std::stringstream ss;

  const UChar* head = reinterpret_cast<const UChar*>(input.c_str());
  const UChar* cur = head, *end = head + input.size();

  OnigRegion* region = onig_region_new();

  // We need to check when cur == end as "$" matches to the eos.
  while (cur <= end) {
    int r = onig_match(reg_, head, end, cur, region, ONIG_OPTION_NONE);
    if (r >= 0) {
      replace(input, region, ss);
      cur += r;
    }
    onig_region_clear(region);
    // If the pattern didn't match or mached an empty string, proceed the
    // pointer forcely.
    if (r <= 0) {
      if (cur < end)
        ss << *cur;
      ++cur;
    }
  }
  onig_region_free(region, 1);

  output = ss.str();
}

}  // namespace fv_converter
}  // namespace core
}  // namespace jubatus
