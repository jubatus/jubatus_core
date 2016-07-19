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

#ifndef JUBATUS_CORE_FV_CONVERTER_CHAR_SPLITTER_HPP_
#define JUBATUS_CORE_FV_CONVERTER_CHAR_SPLITTER_HPP_

#include <string>
#include <utility>
#include <vector>
#include <iostream>
#include "word_splitter.hpp"

namespace jubatus {
namespace core {
namespace fv_converter {
char * subStr(const std::string& cStr, int iStart, int iLength);
int cntByte(unsigned char cChar);
size_t find_first_not_of(const std::string& target, std::vector<std::string> separators, size_t pos);

size_t find_first_of(const std::string& target, std::vector<std::string> separators, size_t pos);

class char_splitter : public word_splitter {
 public:
  explicit char_splitter(const std::string& separator)
  : separator_(separator) {
    int i=0,iCnt=0;
    while (separator[i] != '\0') {
        int cnt = cntByte(separator[i]);
        char* str = subStr(separator,iCnt , 1);
        separators_.push_back(str);

        iCnt++;
        i+= cnt;
    }

  }

  void split(
      const std::string& string,
      std::vector<std::pair<size_t, size_t> >& ret_boundaries) const;

private:

    const std::string separator_;

    std::vector<std::string> separators_;
};

}  // namespace fv_converter
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_FV_CONVERTER_CHAR_SPLITTER_HPP_
