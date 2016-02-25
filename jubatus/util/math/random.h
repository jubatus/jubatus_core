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

#ifndef JUBATUS_UTIL_MATH_RANDOM_H_
#define JUBATUS_UTIL_MATH_RANDOM_H_

#include <cmath>
#include <vector>
#include <map>

#include "../system/time_util.h"
#include "constant.h"
#include "random/mersenne_twister.h"

namespace jubatus {
namespace util{
namespace math{
namespace random{

template <class Gen>
class random{

public:
  random()
    :g(feeder(jubatus::util::system::time::get_clock_time())),
    next_gaussian_stocked(false){
  }
private:
  uint32_t feeder(jubatus::util::system::time::clock_time ct){
    return (uint32_t)(ct.sec)*(uint32_t)1000000+(uint32_t)(ct.usec);
  }
public:
  random(uint32_t seed)
    :g(seed),
    next_gaussian_stocked(false){
  }
  ~random(){
  }

  /// generate [0,0xffffffff] random number
  uint32_t next_int(){
    return g.next();
  }
  
  /// generate [0,\a a) random integer
  uint32_t next_int(uint32_t a){
    return static_cast<uint32_t>(next_double()*a);
  }

  /// generate [\a a,\a b) random integer
  uint32_t next_int(uint32_t a, uint32_t b){
    return a+next_int(b-a);
  }

  /// generates [0,1) random real number with 53bit resolution
  double next_double(){
    uint32_t a=g.next()>>5;
    uint32_t b=g.next()>>6;
    return (a*67108864.0+b)*(1.0/9007199254740992.0);
  }

  /// generate [0,\a a) random real number
  double next_double(double a){
    return a*next_double();
  }

  /// generate [\a a,\a b) random real number
  double next_double(double a, double b){
    return a+next_double(b-a);
  }

  /// generate normalized standard distribution
  double next_gaussian(){
    if(next_gaussian_stocked){
      next_gaussian_stocked=false; 
      return next_gaussian_stock;
    }else{
      double a = 1.0-next_double();
      double b = 1.0-next_double();
      double r1 = std::sqrt(-2.0*std::log(a))*std::sin(2.0*jubatus::util::math::pi*b);
      double r2 = std::sqrt(-2.0*std::log(a))*std::cos(2.0*jubatus::util::math::pi*b);
      next_gaussian_stock=r2;
      next_gaussian_stocked=true;
      return r1;
    }
  }
  
  /// generate standard distribution with specified \a mean and \a deviation
  double next_gaussian(double mean, double deviation){
    return mean + deviation * next_gaussian();
  }

  /// generate gamma distribution Gamma(alpha, 1)
  double next_gamma(double alpha){
    if( alpha<= 1 ){ //Ahrens and Dieter
      double c = jubatus::util::math::napier_e / (alpha + jubatus::util::math::napier_e);
      while(true){ //try until sample is accepted
        double u1 = next_double();
        double u2 = next_double();
        double x;
        if(u1 < c){
          x = pow(u1/c, 1.0/alpha);
          if( log(u2) <= -x ){
            return x;
          }
        }else{
          x = -log((1.0-u1)/(c*alpha));
          if( log(u2) <= (alpha-1)*log(x) ){
            return true;
          }
        }
      }
    }else{ //Marsaglia and Tsang
      double c1 = alpha-1/3.0;
      double c2 = 1.0/sqrt(9.0*c1);
      while(true){ //try until sample is accepted
        double z = next_gaussian();
        if(c2 * z <= -1) continue; //rejected
        double t = (1.0+c2*z);
        double nu = t*t*t;
        double u = next_double();
        if( (u < (1 - 0.0331*z*z*z*z)) ||
            (log(u) < (0.5*z*z + c1*(1.0-nu+log(nu))) )
          ){
          return c1 * nu;
        }
      }
    }
  }

  /// generate gamma distribution Gamma(alpha, beta) (pdf propto x^{alpha-1} e^{-beta*x} )
  double next_gamma(double alpha, double beta){
    return next_gamma(alpha)*beta;
  }

  /// generate beta distribution Beta(alpha, beta) (pdf propto x^{alpha-1} (1-x)^{beta-1} )
  double next_beta(double alpha, double beta){
    double x1 = next_gamma(alpha);
    double x2 = next_gamma(beta);
    return x1/(x1+x2);
  }

  ////mul next_int()
  uint32_t operator()(){
    return next_int();
  }


  ////mul next_int(\a a)
  uint32_t operator()(uint32_t a){
    return next_int(a);
  }

  ////mul next_int(\a a,\a b)
  uint32_t operator()(uint32_t a, uint32_t b){
    return next_int(a,b);
  }

private:
  Gen g;
  bool next_gaussian_stocked; double next_gaussian_stock;
};

typedef random<mersenne_twister> mtrand;

/// select k random integer from range [0,n), allowing multiple occurrence. O(k)
template<typename RAND>
bool sample_with_replacement(RAND& r, int n, int k, std::vector<int>& res) {
  if (n<=0||k<0) return false;
  res.resize(k);
  for (int i=0;i<k;++i) res[i]=r(n);
  return true;
}

/// select k random integer from range [0,n), allowing multiple occurrence. O(k log k)
template<typename RAND>
bool sample_without_replacement(RAND& r, int n, int k, std::vector<int>& res) {
  if (n<=0||k<0||k>n) return false;
  std::map<int,int> mm; // pos -> value
  for (int i=0;i<k;++i) mm[i]=i;
  for (int i=0;i<k;++i) {
    int j=i+r(n-i);
    if (!mm.count(j)) mm[j]=j;
    int t=mm[i];
    mm[i]=mm[j];
    mm[j]=t;
  }
  
  res.resize(k);
  for (std::map<int,int>::iterator it=mm.begin();it!=mm.end();++it) 
    if (0<=it->second&&it->second<k)
      res[it->second]=it->first;
  return true;
}

} // random
} // math
} // util
} // jubatus
#endif // #ifndef JUBATUS_UTIL_MATH_RANDOM_H_
