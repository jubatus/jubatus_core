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

#ifndef JUBATUS_UTIL_LANG_FUNCTION_H_
#define JUBATUS_UTIL_LANG_FUNCTION_H_

#include <functional>
#if defined(__GLIBCXX__) && __cplusplus < 201103
#include <tr1/functional>
#endif

namespace jubatus {
namespace util {
namespace lang {

namespace detail {
#if defined(__GLIBCXX__) && __cplusplus < 201103
namespace function_ns = ::std::tr1;
#else
namespace function_ns = ::std;
#endif
}

template <class F>
class function : public detail::function_ns::function<F> {
  typedef detail::function_ns::function<F> base;

public:
  function() {}
  template <class Fn>
  function (const Fn& f) : base(f) {}

  template <class Fn>
  function& operator=(const Fn& f) {
    base::operator=(f);
    return *this;
  }
};

} // lang
} // util
} // jubatus
#endif // #ifndef JUBATUS_UTIL_LANG_FUNCTION_H_
