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

#ifndef JUBATUS_CORE_COMMON_THREAD_POOL_HPP_
#define JUBATUS_CORE_COMMON_THREAD_POOL_HPP_

#include <jubatus/util/concurrent/condition.h>
#include <jubatus/util/concurrent/mutex.h>
#include <jubatus/util/concurrent/lock.h>
#include <jubatus/util/concurrent/thread.h>
#include <jubatus/util/lang/bind.h>
#include <jubatus/util/lang/function.h>
#include <jubatus/util/lang/shared_ptr.h>

#include <queue>
#include <vector>

namespace jubatus {
namespace core {
namespace common {

class thread_pool {
  typedef jubatus::util::concurrent::mutex mutex;
  typedef jubatus::util::concurrent::condition condition;
  typedef jubatus::util::concurrent::thread thread;
  typedef jubatus::util::concurrent::scoped_lock scoped_lock;

 public:
  explicit thread_pool(int max_threads);
  ~thread_pool();

  template<typename T>
  class future {
  public:
    explicit future(const jubatus::util::lang::function<T()>& func)
      : func_(func), ret_(), mutex_(), cond_(), finished_(false) {}

    const T& get() const {
      scoped_lock lk(mutex_);
      while (!finished_)
        cond_.wait(mutex_);
      return ret_;
    }

  private:
    void execute() {
      ret_ = func_();
      {
        jubatus::util::concurrent::scoped_lock lk(mutex_);
        finished_ = true;
        cond_.notify_all();
      }
    }

    const jubatus::util::lang::function<T()> func_;
    T ret_;
    mutable mutex mutex_;
    mutable condition cond_;
    bool finished_;

    friend class thread_pool;

    future(const future&);
    future& operator =(const future&);
  };

  template<typename Function>
  jubatus::util::lang::shared_ptr<future<typename Function::result_type> >
  async(Function f) {
    using jubatus::util::lang::bind;
    using jubatus::util::lang::shared_ptr;
    typedef typename Function::result_type R;
    shared_ptr<future<R> >fut(new future<R>(f));
    if (pool_.capacity() == 0) {
      fut->execute();
    } else {
      scoped_lock lk(mutex_);
      const bool empty_flag = queue_.empty();
      queue_.push(bind(&wrapper<R>, fut.get()));
      if (pool_.size() < pool_.capacity()) {
        shared_ptr<thread> th(new thread(bind(&worker, this)));
        th->start();
        pool_.push_back(th);
      } else if (empty_flag) {
        cond_.notify_all();
      }
    }
    return fut;
  }

  template<typename Function>
  std::vector<
    jubatus::util::lang::shared_ptr<
      future<typename Function::result_type> > >
  async_all(const std::vector<Function>& funcs) {
    using jubatus::util::lang::bind;
    using jubatus::util::lang::shared_ptr;
    typedef typename Function::result_type R;
    typedef typename std::vector<Function>::const_iterator Iter;
    typedef typename std::vector<shared_ptr<future<R> > >::iterator RIter;

    std::vector<shared_ptr<future<R> > > ret;
    ret.reserve(funcs.size());

    for (Iter it = funcs.begin(); it != funcs.end(); ++it)
      ret.push_back(shared_ptr<future<R> >(new future<R>(*it)));

    if (pool_.capacity() == 0) {
      for (RIter it = ret.begin(); it != ret.end(); ++it)
        (*it)->execute();
    } else {
      scoped_lock lk(mutex_);
      const bool empty_flag = queue_.empty();
      for (RIter it = ret.begin(); it != ret.end(); ++it)
        queue_.push(bind(&wrapper<R>, (*it).get()));
      while (pool_.size() < pool_.capacity() && pool_.size() < queue_.size()) {
        shared_ptr<thread> th(new thread(bind(&worker, this)));
        th->start();
        pool_.push_back(th);
      }
      if (empty_flag)
        cond_.notify_all();
    }
    return ret;
  }

 private:
  template<typename T>
  static void wrapper(future<T> *fut) {
    fut->execute();
  }

  static void worker(thread_pool *tp);

  std::vector<jubatus::util::lang::shared_ptr<thread> > pool_;
  std::queue<jubatus::util::lang::function<void()> > queue_;
  mutex mutex_;
  condition cond_;
  bool shutdown_;
};

namespace default_thread_pool {
  extern thread_pool instance;

  template<typename Function>
  jubatus::util::lang::shared_ptr<
    thread_pool::future<typename Function::result_type> >
  async(Function f) {
    return instance.async(f);
  }

  template<typename Function>
  std::vector<
    jubatus::util::lang::shared_ptr<
      thread_pool::future<typename Function::result_type> > >
  async_all(const std::vector<Function>& funcs) {
    return instance.async_all(funcs);
  }
}

}  // namespace common
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_COMMON_THREAD_POOL_HPP_
