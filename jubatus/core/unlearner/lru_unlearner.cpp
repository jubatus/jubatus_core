// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2013 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#include "lru_unlearner.hpp"

#include <string>
#include "jubatus/util/data/unordered_set.h"

// TODO(kmaehashi) move key_matcher to common
#include "../fv_converter/key_matcher.hpp"
#include "../fv_converter/key_matcher_factory.hpp"
#include "../common/exception.hpp"

using jubatus::util::data::unordered_set;
using jubatus::core::fv_converter::key_matcher_factory;

namespace jubatus {
namespace core {
namespace unlearner {

lru_unlearner::lru_unlearner(const config& conf)
    : max_size_(conf.max_size) {
  if (conf.max_size <= 0) {
    throw JUBATUS_EXCEPTION(
        common::config_exception() << common::exception::error_message(
            "max_size must be a positive integer"));
  }
  entry_map_.reserve(max_size_);

  if (conf.sticky_pattern) {
    key_matcher_factory f;
    sticky_matcher_ = f.create_matcher(*conf.sticky_pattern);
  }
}

bool lru_unlearner::can_touch(const std::string& id) {
  return (exists_in_memory(id) || sticky_ids_.size() < max_size_);
}

bool lru_unlearner::touch(const std::string& id) {
  // When sticky pattern is specified, unlearner excludes IDs matching
  // the pattern from unlearning.
  bool is_sticky = sticky_matcher_ && sticky_matcher_->match(id);

  if (is_sticky) {
    unordered_set<std::string>::const_iterator it = sticky_ids_.find(id);
    if (it != sticky_ids_.end()) {
      // Sticky ID that is already on memory; nothing to do.
      return true;
    }
  } else {
    entry_map::iterator it = entry_map_.find(id);
    if (it != entry_map_.end()) {
      // Non-sticky ID that is already on memory; mark the ID
      // as most recently used.
      lru_.push_front(id);
      lru_.erase(it->second);
      it->second = lru_.begin();
      return true;
    }
  }

  // Touched ID is not on memory; need to secure a space for it.
  if ((entry_map_.size() + sticky_ids_.size()) >= max_size_) {
    // No more space; sticky_ids_ is the list of IDs that cannot be
    // unlearned, so try to unlearn from entry_map_.
    if (entry_map_.size() == 0) {
      // entry_map_ is empty; nothing can be unlearned.
      return false;
    }

    // Unlearn the least recently used entry.
    unlearn(lru_.back());
    entry_map_.erase(lru_.back());
    lru_.pop_back();
  }

  // Register the new ID.
  if (is_sticky) {
    sticky_ids_.insert(id);
  } else {
    lru_.push_front(id);
    entry_map_[id] = lru_.begin();
  }

  return true;
}

bool lru_unlearner::remove(const std::string& id) {
  // Try to erase from non-sticky ID.
  {
    entry_map::iterator it = entry_map_.find(id);
    if (it != entry_map_.end()) {
      lru_.erase(it->second);
      entry_map_.erase(it);
      return true;
    }
  }

  // Try to erase from sticky ID.
  {
    unordered_set<std::string>::iterator it = sticky_ids_.find(id);
    if (it != sticky_ids_.end()) {
      sticky_ids_.erase(it);
      return true;
    }
  }

  return false;
}

bool lru_unlearner::exists_in_memory(const std::string& id) const {
  return entry_map_.count(id) > 0 || sticky_ids_.count(id) > 0;
}

// private

void lru_unlearner::rebuild_entry_map() {
  entry_map_.clear();
  for (lru::iterator it = lru_.begin(); it != lru_.end(); ++it) {
    entry_map_[*it] = it;
  }
}

}  // namespace unlearner
}  // namespace core
}  // namespace jubatus
