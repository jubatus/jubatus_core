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
#include <cfloat>
#include <deque>

#include "../common/assert.hpp"

namespace jubatus {
namespace core {
namespace burst {

class result_storage::impl_ {
  typedef std::deque<result_t> results_t;
  results_t results_;
  size_t results_max_;
  double oldest_start_pos_not_mixed_;

 public:
  explicit impl_(int stored_results_max)
      : results_max_(stored_results_max), oldest_start_pos_not_mixed_(DBL_MAX) {
  }

  void store(const result_t& result, bool merge = false) {
    double result_start_pos = result.get_start_pos();

    if (results_.empty()) {
      results_.push_front(result);
    } else {
      typedef results_t::iterator iterator_t;
      for (iterator_t iter = results_.begin(), end = results_.end();
          iter != end; ++iter) {
        if (iter->has_start_pos_older_than(result_start_pos)) {
          results_.insert(iter, result);
          break;
        } else if (iter->has_same_start_pos_to(result_start_pos)) {
          JUBATUS_ASSERT_EQ(
              iter->get_batch_size(), result.get_batch_size(), "");
          JUBATUS_ASSERT(iter->has_same_batch_interval(result));
          if (merge) {
            bool mixed = iter->mix(result);
            JUBATUS_ASSERT(mixed);
          } else {
            *iter = result;  // update with new result
          }
          break;
        }
      }
    }

    if (result_start_pos < oldest_start_pos_not_mixed_) {
      oldest_start_pos_not_mixed_ = result_start_pos;
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

  diff_t get_diff() const {
    diff_t diff;

    typedef results_t::const_iterator iterator_t;
    for (iterator_t iter = results_.begin(), end = results_.end();
        iter != end; ++iter) {
      if (iter->has_start_pos_older_than(oldest_start_pos_not_mixed_)) {
        break;
      }
      diff.push_back(*iter);
    }

    return diff;
  }

  void put_diff(const diff_t& diff) {
    // merge diff
    for (diff_t::const_iterator iter = diff.begin();
         iter != diff.end(); ++iter) {
      store(*iter, true);
    }

    // clear diff
    oldest_start_pos_not_mixed_ = DBL_MAX;
  }

  MSGPACK_DEFINE(results_, results_max_, oldest_start_pos_not_mixed_);
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

result_storage::diff_t result_storage::get_diff() const {
  JUBATUS_ASSERT(p_);
  return p_->get_diff();
}

void result_storage::put_diff(const result_storage::diff_t& diff) {
  JUBATUS_ASSERT(p_);
  p_->put_diff(diff);
}

void result_storage::pack(framework::packer& packer) const {
  JUBATUS_ASSERT(p_);
  packer.pack(*p_);
}

void result_storage::unpack(msgpack::object o) {
  JUBATUS_ASSERT(p_);
  o.convert(p_.get());
}

}  // namespace burst
}  // namespace core
}  // namespace jubatus
