// Copyright (c)2008-2011, Preferred Infrastructure Inc.
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
// 
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
// 
//     * Neither the name of Preferred Infrastructure nor the names of other
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef JUBATUS_UTIL_LANG_SHARED_PTR_H_
#define JUBATUS_UTIL_LANG_SHARED_PTR_H_

#include <exception>
#include <memory>
#if defined(__GLIBCXX__) && __cplusplus < 201103
#include <tr1/memory>
#endif

namespace jubatus {
namespace util {
namespace concurrent {
namespace threading_model {

class single_thread;

} // threading_model
} // concurrent

namespace lang {

class bad_weak_ptr : public std::exception {
public:
  const char* what() const throw() {
    return "jubatus::util::lang::bad_weak_ptr";
  }
};

template <class T>
class weak_ptr;

template <class T>
class enable_shared_from_this;

namespace detail {
#if defined(__GLIBCXX__) && __cplusplus < 201103
namespace shared_ptr_ns = ::std::tr1;
#else
namespace shared_ptr_ns = ::std;
#endif
}

template <class T, class /* dummy for compatibility */ = jubatus::util::concurrent::threading_model::single_thread>
class shared_ptr : public detail::shared_ptr_ns::shared_ptr<T> {
  typedef detail::shared_ptr_ns::shared_ptr<T> base;

  template <class U>
  friend class enable_shared_from_this;

  template <class U>
  friend class weak_ptr;

  template <class U, class V, class TM>
  friend shared_ptr<U> static_pointer_cast(const shared_ptr<V, TM>& p);
  template <class U, class V, class TM>
  friend shared_ptr<U> dynamic_pointer_cast(const shared_ptr<V, TM>& p);
  template <class U, class V, class TM>
  friend shared_ptr<U> const_pointer_cast(const shared_ptr<V, TM>& p);

public:
  shared_ptr() {}

  template <class U>
  explicit shared_ptr(U* p) : base(p) {}

  template <class U, class Deleter>
  shared_ptr(U* p, Deleter d) : base(p, d) {}

  template <class U, class UM>
  shared_ptr(const shared_ptr<U, UM>& p) : base(p) {}

  template <class U>
  explicit shared_ptr(const weak_ptr<U>& p)
  try : base(p) {

  } catch (detail::shared_ptr_ns::bad_weak_ptr&) {
    throw bad_weak_ptr();
  }

  template <class U>
  explicit shared_ptr(std::auto_ptr<U>& p) : base(p.release()) {}

private:
  template <class U>
  explicit shared_ptr(const detail::shared_ptr_ns::shared_ptr<U>& p) : base(p) {}

public:
  template <class U, class UM>
  shared_ptr& operator=(const shared_ptr<U, UM>& r) {
    base::operator=(r);
    return *this;
  }

  template <class U>
  shared_ptr& operator=(std::auto_ptr<U>& p) {
    return *this = shared_ptr<U>(p);
  }
};

template <class T, class U, class TM>
shared_ptr<T> static_pointer_cast(const shared_ptr<U, TM>& p)
{
  return shared_ptr<T>(detail::shared_ptr_ns::static_pointer_cast<T>(p));
}

template <class T, class U, class TM>
shared_ptr<T> dynamic_pointer_cast(const shared_ptr<U, TM>& p)
{
  return shared_ptr<T>(detail::shared_ptr_ns::dynamic_pointer_cast<T>(p));
}

template <class T, class U, class TM>
shared_ptr<T> const_pointer_cast(const shared_ptr<U, TM>& p)
{
  return shared_ptr<T>(detail::shared_ptr_ns::const_pointer_cast<T>(p));
}

template <class Deleter, class T, class TM>
Deleter* get_deleter(const shared_ptr<T, TM>& p)
{
  return detail::shared_ptr_ns::get_deleter<Deleter>(p);
}

} // lang
} // util
} // jubatus
#endif // #ifndef JUBATUS_UTIL_LANG_SHARED_PTR_H_
