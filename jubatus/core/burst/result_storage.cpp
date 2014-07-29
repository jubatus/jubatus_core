// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2014 Preferred Infrastructure and Nippon Telegraph and Telephone Corporation.
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

#include "result_storage.hpp"

#include <stddef.h>
#include <deque>

#include "../common/assert.hpp"

namespace jubatus {
namespace core {
namespace burst {

class result_storage::impl_ {
  typedef std::deque<result_t> results_t;
  results_t results_;
  size_t results_max_;

 public:
  explicit impl_(int stored_results_max)
      : results_max_(stored_results_max) {
  }

  void store(const result_t& result) {
    if (results_.empty()) {
      results_.push_front(result);
    } else {
      typedef results_t::iterator iterator_t;
      for (iterator_t iter = results_.begin(), end = results_.end();
          iter != end; ++iter) {
        if (iter->is_older_than(result)) {
          results_.insert(iter, result);
          break;
        } else if (iter->has_same_start_pos(result)) {
          JUBATUS_ASSERT_EQ(
              iter->get_batch_size(), result.get_batch_size(), "");
          JUBATUS_ASSERT(iter->has_same_batch_interval(result));
          *iter = result;  // update with new result
          break;
        }
      }
    }

    while (results_.size() > results_max_) {
      results_.pop_back();
    }
  }

  result_t get_latest_result() const {
    if (results_.empty()) {
      return result_t();
    }
    return results_.front();
  }
  result_t get_result_at(double pos) const {
    typedef results_t::const_iterator iterator_t;
    for (iterator_t iter = results_.begin(), end = results_.end();
        iter != end; ++iter) {
      if (iter->contains(pos)) {
        return *iter;
      }
    }
    return result_t();
  }
};

result_storage::result_storage(int stored_results_max)
    : p_(new impl_(stored_results_max)) {
}

result_storage::~result_storage() {
}

void result_storage::store(const result_t& result) {
  JUBATUS_ASSERT(p_);
  p_->store(result);
}

burst_result result_storage::get_latest_result() const {
  JUBATUS_ASSERT(p_);
  return p_->get_latest_result();
}

burst_result result_storage::get_result_at(double pos) const {
  JUBATUS_ASSERT(p_);
  return p_->get_result_at(pos);
}

}  // namespace burst
}  // namespace core
}  // namespace jubatus
