// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2012 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#include <sched.h>
#include <functional>
#include <vector>
#include <queue>
#include "jubatus/util/concurrent/thread.h"
#include "jubatus/util/concurrent/lock.h"
#include "jubatus/util/concurrent/condition.h"
#include "jubatus/util/lang/bind.h"
#include "jubatus/util/lang/function.h"
#include "jubatus/util/lang/shared_ptr.h"

using std::make_pair;
using std::pair;
using std::vector;

namespace jubatus {
namespace core {
namespace storage {

template <typename T>
class blocking_queue : public util::lang::noncopyable {
 public:
  blocking_queue() {}
  void enqueue(const T& item) {
    {
      util::concurrent::scoped_lock lk(lk_);
      const bool was_empty = queue_.empty();
      queue_.push(item);
      if (was_empty) {
        empty_wait_.notify_all();  // notify_one() may be suitable?
      }
    }
  }

  T dequeue() {
    while (true) {
      util::concurrent::scoped_lock lk(lk_);
      if (queue_.empty()) {  // if empty
        empty_wait_.wait(lk_);  // unlock
        //  relock
      }
      if (queue_.empty()) {
        continue;
      }

      T result = queue_.front();
      queue_.pop();
      return result;
    }
  }

  size_t size() const {
    util::concurrent::scoped_lock lk(lk_);
    return queue_.size();
  }

 private:
  mutable util::concurrent::mutex lk_;
  mutable util::concurrent::condition empty_wait_;
  std::queue<T> queue_;
};


template <typename Arg>
class thread_pool {
  struct workingset {
    Arg arg;
    util::lang::function<void(Arg)> func;
    workingset(const Arg& a, util::lang::function<void(Arg)>& f)
      : arg(a),
        func(f)
    {}
  };

  static void task(int worker_id, blocking_queue<workingset>* queue) {
    while (true) {
      workingset w = queue->dequeue();
      w.func(w.arg);
      __sync_synchronize();
    }
  }

 public:
  thread_pool(int num)
    : threads_(), terminate_(false) {
    threads_.reserve(num);
    for (int i = 0; i < num; ++i) {
      threads_.push_back(util::lang::shared_ptr<util::concurrent::thread>(
                             new util::concurrent::thread(
                                 util::lang::bind(task,
                                                  i,
                                                 &queue_))));
      threads_.back()->start();
    }
  }
  void add_task(Arg& arg, util::lang::function<void(Arg)> f) {
    queue_.enqueue(workingset(arg, f));
  }

 private:
  blocking_queue<workingset> queue_;
  std::vector<util::lang::shared_ptr<util::concurrent::thread> > threads_;
  volatile bool terminate_;
};

}  // namespace storage
}  // namespace core
}  // namespace jubatus
