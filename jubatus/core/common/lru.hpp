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

#ifndef JUBATUS_CORE_COMMON_LRU_HPP_
#define JUBATUS_CORE_COMMON_LRU_HPP_

#include <utility>
#include <stdexcept>
#include "jubatus/util/data/unordered_map.h"

namespace jubatus {
namespace core {
namespace common {

// Least Recently Used Cache in O(log(n))
// *** thread unsafe ***

template <class K, class V>
class lru {
  struct Node {
    Node *prev;
    Node *next;
    K key;
    V value;
  };
  typedef jubatus::util::data::unordered_map<K, Node*> map_type;

 public:
  explicit lru(int size) : max_size_(size), map_(), head_(NULL), tail_(NULL) {
  }
  ~lru() {
    clear();
  }

  bool has(const K &key) const {
    return map_.find(key) != map_.end();
  }

  const V &get(const K &key) {
    typename map_type::iterator it = map_.find(key);
    if (it == map_.end())
      throw std::runtime_error("lru::get(): key is not found");
    touch(it->second);
    return it->second->value;
  }

  void set(const K &key, const V &val) {
    if (has(key)) return;

    while (static_cast<int>(map_.size()) >= max_size_ && head_)
      remove(head_->key);

    Node *node = new Node();
    node->key = key;
    node->value = val;
    node->prev = tail_;
    node->next = NULL;
    map_.insert(std::pair<K, Node*>(key, node));
    if (!head_)
      head_ = node;
    if (tail_)
      tail_->next = node;
    tail_ = node;
  }

  void touch(const K &key) {
    typename map_type::iterator it = map_.find(key);
    if (it != map_.end())
      touch(it->second);
  }

  void remove(const K &key) {
    typename map_type::const_iterator it = map_.find(key);
    if (it != map_.end()) {
      Node *node = it->second;
      map_.erase(it);
      remove_link(node);
      delete node;
    }
  }

  void clear() {
    for (typename map_type::iterator it = map_.begin();
         it != map_.end(); ++it) {
      delete it->second;
    }
    map_.clear();
    head_ = tail_ = NULL;
  }

  V &operator[](const K &key) {
    typename map_type::iterator it = map_.find(key);
    if (it != map_.end()) {
      touch(it->second);
      return it->second->value;
    }
    set(key, V());
    return map_.find(key)->second->value;
  }

 private:
  inline void remove_link(Node *node) {
    if (node->prev) {
      node->prev->next = node->next;
    } else if (head_ == node) {
      head_ = node->next;
    }
    if (node->next) {
      node->next->prev = node->prev;
    } else if (tail_ == node) {
      tail_ = node->prev;
    }
  }
  inline void touch(Node *node) {
    remove_link(node);
    if (tail_)
      tail_->next = node;
    node->prev = tail_;
    node->next = NULL;
    tail_ = node;
  }

  int max_size_;
  map_type map_;
  Node *head_;
  Node *tail_;
};
}  // common
}  // core
}  // jubatus
#endif  // JUBATUS_CORE_COMMON_LRU_HPP_
