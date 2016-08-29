// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2011,2012 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#ifndef JUBATUS_CORE_COMMON_EXCEPTION_INFO_HPP_
#define JUBATUS_CORE_COMMON_EXCEPTION_INFO_HPP_

#include <cstring>
#include <string>
#include <typeinfo>
#include "jubatus/util/lang/shared_ptr.h"
#include "jubatus/util/lang/cast.h"
#include "jubatus/util/lang/demangle.h"

namespace jubatus {
namespace core {
namespace common {
namespace exception {

#define DEFINE_ERROR_TAG(tagName, tagText, argType)             \
  struct error_tag_##tagName {                                  \
    const std::string name() { return tagText; }                \
  };                                                            \
  typedef jubatus::core::common::exception::error_info          \
      <error_tag_##tagName, argType > tagName;

class error_info_base {
 public:
  virtual bool splitter() const {
    return false;
  }

  virtual std::string tag() const = 0;
  virtual std::string as_string() const = 0;

  virtual ~error_info_base() throw () {
  }
};

template<class Tag, class V>
class error_info;

template<class Tag, class V>
inline std::string to_string(const error_info<Tag, V>& info) {
  return jubatus::util::lang::lexical_cast<std::string, V>(info.value());
}

template<class Tag, class V>
class error_info : public error_info_base {
 public:
  typedef V value_type;
  explicit error_info(value_type v);
  ~error_info() throw ();

  std::string tag() const;
  std::string as_string() const;

  value_type value() const {
    return value_;
  }

 private:
  value_type value_;
};

template<class Tag, class V>
inline error_info<Tag, V>::error_info(value_type v)
    : value_(v) {
}

template<class Tag, class V>
inline error_info<Tag, V>::~error_info() throw () {
}

template<class Tag, class V>
inline std::string error_info<Tag, V>::tag() const {
  return Tag().name();
}

template<class Tag, class V>
inline std::string error_info<Tag, V>::as_string() const {
  return to_string(*this);
}

}  // namespace exception
}  // namespace common
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_COMMON_EXCEPTION_INFO_HPP_
