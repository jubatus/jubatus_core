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

namespace jubatus {
namespace core {
namespace fv_converter {

void char_splitter::split(
    const std::string& string,
    std::vector<std::pair<size_t, size_t> >& ret_boundaries) const {
  std::vector<std::pair<size_t, size_t> > bounds;

  size_t last = 0;
  while (true) {
    size_t begin = find_first_not_of(string,separators_, last);
    if (begin == std::string::npos) {
      break;
    }

    size_t end = find_first_of(string,separators_, begin);
    if (end == std::string::npos) {
      size_t len = string.size() - begin;
      bounds.push_back(std::make_pair(begin, len));
      break;
    } else {
      size_t len = end - begin;
      bounds.push_back(std::make_pair(begin, len));
      last = end;
    }
  }

  bounds.swap(ret_boundaries);
}

int cntByte(unsigned char cChar)
{
  int iByte;

  if ((cChar >= 0x00) && (cChar <= 0x7f)) {
    iByte = 1;
  } else if ((cChar >= 0xc2) && (cChar <= 0xdf)) {
    iByte = 2;
  } else if ((cChar >= 0xe0) && (cChar <= 0xef)) {
    iByte = 3;
  } else if ((cChar >= 0xf0) && (cChar <= 0xf7)) {
    iByte = 4;
  } else if ((cChar >= 0xf8) && (cChar <= 0xfb)) {
    iByte = 5;
  } else if ((cChar >= 0xfc) && (cChar <= 0xfd)) {
    iByte = 6;
  } else {
    iByte = 0;
  }

  return iByte;
}

char* subStr(const std::string& cStr, int iStart, int iLength)
{
  static char cRes[1024];
  char* pRes = cRes;
  int i = 0, iPos = 0;
  int iByte;

  while (cStr[i] != '\0') {
    iByte = cntByte(cStr[i]);
    if (iStart <= iPos && iPos < iStart + iLength) {
      memcpy(pRes, &cStr[i], iByte);
      pRes += iByte;
    }
    i += iByte;
    iPos++;
  }
  *pRes = '\0';

  return cRes;
}

size_t  find_first_not_of(const std::string& target , std::vector<std::string> separators, size_t pos) {
  int i=pos,iPos = 0;
  int iByte;
  bool isSameString;

  while(target[i] != '\0') {
    iByte = cntByte(target[i]);
    char cRes[1024];
    char* pRes = cRes;

    memcpy(pRes, &target[i], iByte);
    pRes += iByte;
    *pRes = '\0';

    isSameString = false;
    for (std::string separator : separators) {
      if(strcmp(separator.c_str() , cRes) == 0){
        isSameString = true;
        break;
      }
    }
    if(!isSameString) {
      return i;
    }

    i += iByte;
    iPos++;
  }
  return std::string::npos;
}

size_t  find_first_of(const std::string& target , std::vector<std::string> separators, size_t pos) {
  int i=pos,iPos = 0;
  int iByte;
  bool isSameString;

  while(target[i] != '\0') {
    iByte = cntByte(target[i]);
    char cRes[1024];
    char* pRes = cRes;

    memcpy(pRes, &target[i], iByte);
    pRes += iByte;
    *pRes = '\0';

    isSameString = false;
    for (std::string separator : separators) {
      if(strcmp(separator.c_str() , cRes) == 0){
        isSameString = true;
        break;
      }
    }
    if(isSameString) {
      return i;
    }

    i += iByte;
    iPos++;
  }
  return std::string::npos;
}


}  // namespace fv_converter
}  // namespace core
}  // namespace jubatus
