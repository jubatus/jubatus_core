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

#ifndef JUBATUS_UTIL_DATA_SERIALIZATION_TR1_UNORDERED_SET_H_
#define JUBATUS_UTIL_DATA_SERIALIZATION_TR1_UNORDERED_SET_H_

#if defined(__GLIBCXX__) && __cplusplus < 201103

#include "base.h"

#include <utility> // include a lightweight header for __GLIBCXX__.
#include <tr1/unordered_set>

#include "pair.h"

namespace jubatus {
namespace util{
namespace data{
namespace serialization{

template <class Archive, class T, class H, class P, class A>
void serialize(Archive &ar, std::tr1::unordered_set<T, H, P, A> &s)
{
  uint32_t size=static_cast<uint32_t>(s.size());
  ar & size;

  if (ar.is_read){
    s.clear();
    while(size--){
      T v;
      ar & v;
      s.insert(v);
    }
  }
  else{
    for (typename std::tr1::unordered_set<T,H,P,A>::iterator p=s.begin();
	 p!=s.end();p++){
      T v(*p);
      ar & v;
    }
  }
}

} // serialization
} // data
} // util
} // jubatus

#endif // #if defined(__GLIBCXX__) && __cplusplus < 201103
#endif // #ifndef JUBATUS_UTIL_DATA_SERIALIZATION_TR1_UNORDERED_SET_H_
