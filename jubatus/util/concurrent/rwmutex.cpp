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

#include "rwmutex.h"

#include <pthread.h>

#ifdef JUBATUS_UTIL_CONCURRENT_RWMUTEX_ERRORCHECK
#include <assert.h>
#include <set>
#include "mutex.h"
#endif

#include "thread.h"
#include "internal.h"
#include "../system/time_util.h"

using namespace std;

using namespace jubatus::util::system::time;

namespace jubatus {
namespace util{
namespace concurrent{

class rw_mutex::impl{
public:
  impl();
  ~impl();

  bool read_lock();
  bool read_lock(double sec);
  bool write_lock();
  bool write_lock(double sec);

  bool unlock();

private:
  pthread_rwlock_t lk;
  bool valid;
#ifdef JUBATUS_UTIL_CONCURRENT_RWMUTEX_ERRORCHECK
  // A set of threads holding read lock or write lock.
  // Before modifying the set, holders_lk must be taken.
  std::set<thread::tid_t> holders;
  mutex holders_lk;
#endif
};

rw_mutex::impl::impl()
  :valid(false)
{
  pthread_rwlockattr_t attr;
  pthread_rwlockattr_init(&attr);
#ifdef PTHREAD_RWLOCK_WRITER_NONRECURSIVE_INITIALIZER_NP
  // switch to writer-preferred lock on linux
  pthread_rwlockattr_setkind_np(&attr,
                                PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
#endif
  int res=pthread_rwlock_init(&lk,&attr);
  if (res==0) valid=true;
}

rw_mutex::impl::~impl()
{
  if (valid){
    // if lock is not released,
    // it may fail by EBUSY...
    // DO NOT DO THAT
    (void)pthread_rwlock_destroy(&lk);
  }
}

bool rw_mutex::impl::read_lock()
{
  if (!valid) return false;
#ifdef JUBATUS_UTIL_CONCURRENT_RWMUTEX_ERRORCHECK
  assert(holders_lk.lock()==true);
  assert(holders.count(thread::id())==0);
#endif
  bool result = pthread_rwlock_rdlock(&lk)==0;
#ifdef JUBATUS_UTIL_CONCURRENT_RWMUTEX_ERRORCHECK
  if (result) holders.insert(thread::id());
  assert(holders_lk.unlock()==true);
#endif
  return result;
}

bool rw_mutex::impl::read_lock(double sec)
{
#ifdef __linux__
  if (!valid) return false;
#ifdef JUBATUS_UTIL_CONCURRENT_RWMUTEX_ERRORCHECK
  assert(holders_lk.lock()==true);
  assert(holders.count(thread::id())==0);
#endif

  bool result;
  if (sec<1e-9) {
    result = pthread_rwlock_tryrdlock(&lk);
  } else {
    timespec end=to_timespec(get_clock_time()+sec);
    result = pthread_rwlock_timedrdlock(&lk, &end)==0;
  }
#ifdef JUBATUS_UTIL_CONCURRENT_RWMUTEX_ERRORCHECK
  if (result) holders.insert(thread::id());
  assert(holders_lk.unlock()==true);
#endif
  return result;
#else
  return false;
#endif
}

bool rw_mutex::impl::write_lock()
{
  if (!valid) return false;
#ifdef JUBATUS_UTIL_CONCURRENT_RWMUTEX_ERRORCHECK
  assert(holders_lk.lock()==true);
  assert(holders.count(thread::id())==0);
#endif
  bool result = pthread_rwlock_wrlock(&lk)==0;
#ifdef JUBATUS_UTIL_CONCURRENT_RWMUTEX_ERRORCHECK
  if (result) holders.insert(thread::id());
  assert(holders_lk.unlock()==true);
#endif
  return result;
}

bool rw_mutex::impl::write_lock(double sec)
{
#if defined(__linux__) || defined(__sparcv8) || defined(__sparcv9)
  if (!valid) return false;
#ifdef JUBATUS_UTIL_CONCURRENT_RWMUTEX_ERRORCHECK
  assert(holders_lk.lock()==true);
  assert(holders.count(thread::id())==0);
#endif

  bool result;
  if (sec<1e-9) {
    result = pthread_rwlock_trywrlock(&lk)==0;
  } else {
    timespec end=to_timespec(get_clock_time()+sec);
    result = pthread_rwlock_timedwrlock(&lk, &end)==0;
  }
#ifdef JUBATUS_UTIL_CONCURRENT_RWMUTEX_ERRORCHECK
  if (result) holders.insert(thread::id());
  assert(holders_lk.unlock()==true);
#endif
  return result;
#else
  return false;
#endif
}

bool rw_mutex::impl::unlock()
{
  if (!valid) return false;
  bool result = pthread_rwlock_unlock(&lk)==0;
#ifdef JUBATUS_UTIL_CONCURRENT_RWMUTEX_ERRORCHECK
  if (result) {
    assert(holders_lk.lock()==true);
    holders.erase(thread::id());
    assert(holders_lk.unlock()==true);
  }
#endif
  return result;
}

rw_mutex::rw_mutex()
  :pimpl(new impl())
{
}

rw_mutex::~rw_mutex()
{
}

bool rw_mutex::read_lock()
{
  return pimpl->read_lock();
}

bool rw_mutex::read_lock(double sec)
{
  return pimpl->read_lock(sec);
}

bool rw_mutex::write_lock()
{
  return pimpl->write_lock();
}

bool rw_mutex::write_lock(double sec)
{
  return pimpl->write_lock(sec);
}

bool rw_mutex::unlock()
{
  return pimpl->unlock();
}

} // concurrent
} // util
} // jubatus
