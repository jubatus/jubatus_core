// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2011 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#include "bit_index_storage.hpp"
#include <functional>
#include <iterator>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include "fixed_size_heap.hpp"

using std::greater;
using std::istringstream;
using std::ostringstream;
using std::make_pair;
using std::pair;
using std::string;
using std::vector;

namespace jubatus {
namespace core {
namespace storage {

bit_index_storage::bit_index_storage() {
}

bit_index_storage::~bit_index_storage() {
}

void bit_index_storage::set_row(const string& row, const bit_vector& bv) {
  bitvals_diff_[row] = bv;
}

void bit_index_storage::get_row(const string& row, bit_vector& bv) const {
  {
    // First find the row in the diff table.
    bit_table_t::const_iterator it = bitvals_diff_.find(row);
    if (it != bitvals_diff_.end() && it->second.bit_num() != 0) {
      // Row found, and is not 0-bit.  0-bit rows in the diff table means
      // that the row has been removed but not MIXed yet.
      bv = it->second;
      return;
    }
  }
  {
    // Next we find the row in the master table.
    bit_table_t::const_iterator it = bitvals_.find(row);
    if (it != bitvals_.end()) {
      bv = it->second;
      return;
    }
  }
  bv = bit_vector();
}

void bit_index_storage::remove_row(const string& row) {
  if (bitvals_.find(row) == bitvals_.end()) {
    // The row is not in the master table; we can
    // immedeately remove it from the diff table.
    bitvals_diff_.erase(row);
  } else {
    // The row is in the master table; we keep the row as
    // 0-bit bit_vector in the diff table until next MIX to
    // propagate the removal of this row to other nodes.
    bitvals_diff_[row] = bit_vector();
  }
}

void bit_index_storage::clear() {
  bit_table_t().swap(bitvals_);
  bit_table_t().swap(bitvals_diff_);
}

void bit_index_storage::get_all_row_ids(std::vector<std::string>& ids) const {
  ids.clear();

  // Collect rows from diff table.
  for (bit_table_t::const_iterator it = bitvals_diff_.begin();
      it != bitvals_diff_.end(); ++it) {
    // Exclude removed (0-bit) rows in diff table.
    if (it->second.bit_num() != 0) {
      ids.push_back(it->first);
    }
  }

  // Collect rows from master table.
  for (bit_table_t::const_iterator it = bitvals_.begin(); it != bitvals_.end();
      ++it) {
    // Exclude rows overwritten in diff table.
    if (bitvals_diff_.find(it->first) == bitvals_diff_.end()) {
      ids.push_back(it->first);
    }
  }
}

void bit_index_storage::get_diff(bit_table_t& diff) const {
  diff = bitvals_diff_;
}

bool bit_index_storage::put_diff(
    const bit_table_t& mixed_diff) {
  for (bit_table_t::const_iterator it = mixed_diff.begin();
      it != mixed_diff.end(); ++it) {
    if (it->second.bit_num() == 0) {
      // 0-bit bit_vector was propagated from other nodes.  This indicates
      // that the row should be removed globally from the master table.
      if (unlearner_) {
        unlearner_->remove(it->first);
      }
      bitvals_.erase(it->first);
    } else {
      if (unlearner_) {
        if (unlearner_->can_touch(it->first)) {
          unlearner_->touch(it->first);
        } else {
          continue;  // drop untouchable value
        }
      }
      bitvals_[it->first] = it->second;
    }
  }

  // New empty rows were created by unlearner and remove_row
  // between get_diff and put_diff
  std::vector<std::string> removed_ids;
  for (bit_table_t::const_iterator it = bitvals_diff_.begin();
      it != bitvals_diff_.end(); ++it) {
    if (it->second.bit_num() == 0) {
      bit_table_t::const_iterator pos;
      pos = mixed_diff.find(it->first);
      if (pos == mixed_diff.end() || pos->second.bit_num() != 0) {
        removed_ids.push_back(it->first);
      }
    }
  }

  bitvals_diff_.clear();

  // Keep empty rows in the diff area until next MIX to
  // propagate the removal of this data to other nodes.
  for (size_t i = 0; i < removed_ids.size(); ++i) {
    bitvals_diff_[removed_ids[i]] = bit_vector();
  }

  return true;
}

void bit_index_storage::mix(const bit_table_t& lhs, bit_table_t& rhs) const {
  for (bit_table_t::const_iterator it = lhs.begin(); it != lhs.end(); ++it) {
    rhs[it->first] = it->second;
  }
}

typedef fixed_size_heap<pair<uint64_t, string>,
    greater<pair<uint64_t, string> > > heap_type;

static void similar_row_one(
    const bit_vector& x,
    const pair<string, bit_vector>& y,
    heap_type& heap) {
  uint64_t match_num = x.calc_hamming_similarity(y.second);
  heap.push(make_pair(match_num, y.first));
}

void bit_index_storage::similar_row(
    const bit_vector& bv,
    vector<pair<string, float> >& ids,
    uint64_t ret_num) const {
  ids.clear();
  uint64_t bit_num = bv.bit_num();
  if (bit_num == 0) {
    return;
  }

  heap_type heap(ret_num);

  for (bit_table_t::const_iterator it = bitvals_diff_.begin();
      it != bitvals_diff_.end(); ++it) {
    // Exclude removed (0-bit) rows in diff table.
    if (it->second.bit_num() != 0) {
      similar_row_one(bv, *it, heap);
    }
  }
  for (bit_table_t::const_iterator it = bitvals_.begin(); it != bitvals_.end();
      ++it) {
    if (bitvals_diff_.find(it->first) != bitvals_diff_.end()) {
      continue;
    }
    similar_row_one(bv, *it, heap);
  }

  vector<pair<uint64_t, string> > scores;
  heap.get_sorted(scores);
  for (size_t i = 0; i < scores.size() && i < ret_num; ++i) {
    ids.push_back(make_pair(scores[i].second,
                            static_cast<float>(scores[i].first) / bit_num));
  }
}

void bit_index_storage::pack(framework::packer& packer) const {
  packer.pack(*this);
}

void bit_index_storage::unpack(msgpack::object o) {
  o.convert(this);
}

string bit_index_storage::name() const {
  return string("bit_index_storage");
}

}  // namespace storage
}  // namespace core
}  // namespace jubatus
