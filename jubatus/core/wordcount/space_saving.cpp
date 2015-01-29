// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2015 Preferred Infrastructure and Nippon Telegraph and Telephone Corporation.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License version 2.1 as published by the Free Software Foundation.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA	02110-1301	USA

#include "space_saving.hpp"
#include <algorithm>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "jubatus/util/lang/cast.h"
#include "jubatus/util/lang/shared_ptr.h"
#include "jubatus/util/concurrent/mutex.h"
#include "jubatus/util/concurrent/lock.h"

#include "../common/exception.hpp"
#include "../common/jsonconfig.hpp"

using std::pair;
using std::string;
using std::vector;
using std::make_pair;
using std::push_heap;
using std::pop_heap;
using std::sort_heap;
using jubatus::util::text::json::to_json;
using jubatus::util::lang::lexical_cast;
using jubatus::util::lang::shared_ptr;
using jubatus::util::data::unordered_map;
using jubatus::util::concurrent::mutex;
using jubatus::util::concurrent::scoped_lock;

namespace jubatus {
namespace core {
namespace wordcount {

class space_saving::space_saving_impl {
	class space_saving_bucket {
		typedef std::list<std::string> lru_t;
		typedef jubatus::util::data::unordered_map<size_t, lru_t> min_count_t;
		struct counter {
			counter() {}
			counter(size_t c) : count_(c) {}
			counter(size_t c, lru_t::iterator it)
				: count_(c), lru_it_(it) { }
			size_t count_;
			lru_t::iterator lru_it_;
		};
		typedef jubatus::util::data::unordered_map<std::string, counter> counter_t;

	 public:
		space_saving_bucket(size_t capacity)
				: counter_(0),
          capacity_(capacity),
					min_(1) {
		}
		void add(const std::string& key, size_t num = 1) {
			// update count
			if (counter_.find(key) == counter_.end()) {
				if (capacity_ == counter_.size()) {
					// no-space left
					counter_t::iterator lowest = find_lowest_key();
					const size_t min_counter = lowest->second.count_;
					lru_erase(min_counter, lowest->second.lru_it_);
					counter_.erase(lowest);
					lru_t::iterator new_it = insert(min_counter + num, key);
					counter_.insert(std::make_pair(key, counter(min_counter + num, new_it)));
				} else {
					// has a room
					lru_t::iterator new_it = insert(num, key);
					counter_.insert(std::make_pair(key, counter(num, new_it)));
				}
			} else {
				// only count up
				counter_t::iterator found = counter_.find(key);
				const size_t before = found->second.count_;
				found->second.count_ += num;
				found->second.lru_it_ = lru_update(before, before + num, found->second.lru_it_);
			}
		}


    vector<pair<string, size_t> > get_ranking(size_t rank_size) const {
      vector<pair<string, size_t> > ranking;
      size_t actual_size = std::min(rank_size, counter_.size());
      ranking.reserve(actual_size + 1);

      size_t pushed = 0;
      counter_t::const_iterator it = counter_.begin();
      for (; pushed < actual_size; ++pushed) {
        ranking.push_back(make_pair(it->first, it->second.count_));
        ++it;
      }
      if (it != counter_.end()) {
        make_heap(ranking.begin(), ranking.end(), entry_compare());
        for (; it != counter_.end(); ++it) {
          ranking.push_back(make_pair(it->first, it->second.count_));
          push_heap(ranking.begin(), ranking.end(), entry_compare());
          pop_heap(ranking.begin(), ranking.end(), entry_compare());
          ranking.pop_back();
        }
        sort_heap(ranking.begin(),ranking.end(), entry_compare());
      }
      return ranking;
    }

    size_t count(const string& key) const {
      counter_t::const_iterator it = counter_.find(key);
      if (it == counter_.end()) {
        return 0;
      }
      return it->second.count_;
    }

   private:

    typedef pair<string, size_t> rank_entry;
    struct entry_compare {
      bool operator()(const rank_entry& lhs, const rank_entry& rhs) {
        return lhs.second > rhs.second;
      }
    };

		counter_t::iterator find_lowest_key() {
			min_count_t::iterator min_it;
			for (;;) {
				min_it = min_count_.find(min_);
				if (min_it == min_count_.end()) {
				} else if (min_it->second.empty()) {
					min_count_.erase(min_it);
				} else {
					break;
				}
				++min_;
			}
			const std::string& min_key = min_it->second.back();
			counter_t::iterator it = counter_.find(min_key);
			return it;
		}

		lru_t::iterator lru_update(size_t before, size_t after, lru_t::iterator it) {
			min_count_[after].push_front(*it);
			lru_erase(before, it);
			return min_count_.find(after)->second.begin();
		}
		void lru_erase(size_t count, lru_t::iterator it) {
			min_count_[count].erase(it);
		}
		lru_t::iterator insert(size_t count, const std::string& value) {
			min_count_[count].push_front(value);
			return min_count_[count].begin();
		}

		counter_t counter_;
		size_t capacity_;

    min_count_t min_count_;
		size_t min_;
	};

 public:
	explicit space_saving_impl(const wordcount_config& conf)
    : capacity_(conf.capacity){
	}
	~space_saving_impl() {}

	bool append(const std::string& bucket,
							const common::sfv_t& datum) {
    if (datum.size() == 0) {
      return false;
    }
		scoped_lock lk(data_mutex_);
		buckets_t::iterator it = data_.find(bucket);
		if (it == data_.end()) {
			data_.insert(make_pair(bucket, space_saving_bucket(capacity_)));
      it = data_.find(bucket);
    }
    for (size_t i = 0; i < datum.size(); ++i) {
      it->second.add(datum[i].first);
    }
    return true;
	}

	size_t count(const std::string& bucket, const std::string& word) const {
    scoped_lock lk(data_mutex_);
    buckets_t::const_iterator it = data_.find(bucket);
    if (it == data_.end()) {
      return 0;
    }
    return it->second.count(word);
	}

  vector<pair<std::string, size_t> >
			get_ranking(const std::string& bucket, size_t size) const {
    scoped_lock lk(data_mutex_);
		buckets_t::const_iterator it = data_.find(bucket);
    if (it == data_.end()) {
      return vector<pair<std::string, size_t> >();
    }
    return it->second.get_ranking(size);
	}

	void clear() {
    data_.clear();
	}

	bool clear_bucket(const std::string& bucket) {
    scoped_lock lk(data_mutex_);
    return data_.erase(bucket) == 1;
	}

 private:
	typedef unordered_map<string, space_saving_bucket> buckets_t;
	buckets_t data_;
	mutable mutex data_mutex_;
  size_t capacity_;
};

space_saving::space_saving(const wordcount_config& conf)
		: impl_(new space_saving_impl(conf)) {}

bool space_saving::append(const std::string& bucket,
						const common::sfv_t& datum) {
	return impl_->append(bucket, datum);
}

size_t space_saving::count(const std::string& bucket,
                           const std::string& word) const {
	return impl_->count(bucket, word);
}

vector<pair<std::string, size_t> >
		space_saving::get_ranking(const std::string& bucket, size_t size) const {
	return impl_->get_ranking(bucket, size);
}

void space_saving::clear() {
  impl_->clear();
}

bool space_saving::clear_bucket(const std::string& bucket) {
	return impl_->clear_bucket(bucket);
}

void space_saving::pack(core::framework::packer& packer) const {
}

void space_saving::unpack(msgpack::object obj) {
}

}	 // namespace wordcount
}	 // namespace core
}	 // namespace jubatus
