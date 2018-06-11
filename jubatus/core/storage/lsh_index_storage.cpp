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

#include "lsh_index_storage.hpp"
#include <cmath>
#include <algorithm>
#include <utility>
#include <sstream>
#include <string>
#include <vector>
#include "jubatus/util/data/unordered_map.h"
#include "jubatus/util/data/unordered_set.h"
#include "jubatus/util/lang/cast.h"
#include "jubatus/util/math/random.h"
#include "lsh_util.hpp"

using std::copy;
using std::ostream;
using std::ostringstream;
using std::istream;
using std::istringstream;
using std::make_pair;
using std::pair;
using std::string;
using std::vector;
using std::sort;
using std::partial_sort;
using std::lower_bound;
using jubatus::util::data::unordered_map;
using jubatus::util::data::unordered_set;
using jubatus::util::math::random::mtrand;

namespace jubatus {
namespace core {
namespace storage {

namespace {

struct greater_second {
  template <typename P>
  bool operator()(const P& l, const P& r) const {
    return l.second > r.second;
  }
};

uint64_t hash_lv(const lsh_vector& lv) {
  uint64_t hash = 14695981039346656037LLU;
  for (size_t i = 0; i < lv.size(); ++i) {
    for (int j = 0; j < 32; j += 8) {
      hash *= 1099511628211LLU;
      hash ^= (static_cast<uint32_t>(lv.get(i)) >> j) & 0xff;
    }
  }
  return hash;
}

void initialize_shift(uint32_t seed, vector<float>& shift) {
  mtrand rnd(seed);
  for (size_t i = 0; i < shift.size(); ++i) {
    shift[i] = rnd.next_double();
  }
}

vector<float> shift_hash(
    const vector<float>& hash,
    const vector<float>& shift) {
  vector<float> shifted(hash);
  for (size_t i = 0; i < shifted.size(); ++i) {
    shifted[i] += shift[i];
  }
  return shifted;
}

bit_vector binarize(const vector<float>& hash) {
  bit_vector bv;
  bv.resize_and_clear(hash.size());
  for (size_t i = 0; i < hash.size(); ++i) {
    if (hash[i] > 0) {
      bv.set_bit(i);
    }
  }
  return bv;
}

float calc_euclidean_distance(
    const lsh_entry& entry,
    const bit_vector& bv,
    double norm) {
  const uint64_t hamm = bv.calc_hamming_similarity(entry.simhash_bv);
  if (hamm == bv.bit_num()) {
    // Avoid NaN caused by arithmetic error
    return std::fabs(norm - entry.norm);
  }
  const double angle = (1 - static_cast<double>(hamm) / bv.bit_num()) * M_PI;
  const double dot = entry.norm * norm * std::cos(angle);
  return std::sqrt(norm * norm + entry.norm * entry.norm - 2 * dot);
}

void retrieve_hit_rows_from_table(
    uint64_t hash,
    const lsh_table_t& table,
    unordered_set<uint64_t>& cands) {
  lsh_table_t::const_iterator it = table.find(hash);
  if (it != table.end()) {
    const vector<uint64_t>& range = it->second;
    for (size_t j = 0; j < range.size(); ++j) {
      cands.insert(range[j]);
    }
  }
}

}  // namespace

lsh_index_storage::lsh_index_storage() {
}

lsh_index_storage::lsh_index_storage(
    size_t lsh_num,
    size_t table_num,
    uint32_t seed)
    : shift_(lsh_num * table_num),
      table_num_(table_num) {
  initialize_shift(seed, shift_);
}

lsh_index_storage::lsh_index_storage(
    size_t table_num,
    const vector<float>& shift)
    : shift_(shift),
      table_num_(table_num) {
}

lsh_index_storage::~lsh_index_storage() {
}

void lsh_index_storage::set_row(
    const string& row,
    const vector<float>& hash,
    double norm) {
  lsh_master_table_t::iterator it = remove_and_get_row(row);
  if (it == master_table_diff_.end()) {
    it = master_table_diff_.insert(make_pair(row, lsh_entry())).first;
  }
  make_entry(hash, norm, it->second);

  const uint64_t id = key_manager_.get_id(row);
  const vector<uint64_t>& lsh_hash = it->second.lsh_hash;
  for (size_t i = 0; i < lsh_hash.size(); ++i) {
    vector<uint64_t>& range = lsh_table_diff_[lsh_hash[i]];
    vector<uint64_t>::iterator it = lower_bound(range.begin(), range.end(), id);
    if (it == range.end() || id != *it) {
      range.insert(it, id);
    }
  }
}

void lsh_index_storage::remove_row(const string& row) {
  const uint64_t row_id = key_manager_.get_id_const(row);
  if (row_id == common::key_manager::NOTFOUND) {
    // Non-existence row
    return;
  }

  lsh_master_table_t::iterator entry_it = master_table_.find(row);
  if (entry_it == master_table_.end()) {
    // Since the row is not yet mixed, it can be immediately erased.
    master_table_diff_.erase(row);
    return;
  }

  // Otherwise, keep the row with empty entry until next MIX.
  master_table_diff_[row] = lsh_entry();
  lsh_entry& entry = entry_it->second;
  put_empty_entry(row_id, entry);

  return;
}

void lsh_index_storage::clear() {
  lsh_master_table_t().swap(master_table_);
  lsh_master_table_t().swap(master_table_diff_);
  lsh_table_t().swap(lsh_table_);
  lsh_table_t().swap(lsh_table_diff_);
  key_manager_.clear();
}

void lsh_index_storage::get_all_row_ids(vector<string>& ids) const {
  const size_t size_upper_bound = master_table_.size()
      + master_table_diff_.size();

  unordered_set<std::string> id_set;
  // equivalent to id_set.reserve(size_upper_bound) in C++11
  id_set.rehash(std::ceil(size_upper_bound / id_set.max_load_factor()));

  // Collect rows from diff table.
  for (lsh_master_table_t::const_iterator it = master_table_diff_.begin();
      it != master_table_diff_.end(); ++it) {
    // Exclude removed (empty) rows in diff table.
    if (!it->second.lsh_hash.empty()) {
      id_set.insert(it->first);
    }
  }

  // Collect rows from master table.
  for (lsh_master_table_t::const_iterator it = master_table_.begin();
      it != master_table_.end(); ++it) {
    // Exclude rows overwritten in diff table.
    if (master_table_diff_.find(it->first) == master_table_diff_.end()) {
      id_set.insert(it->first);
    }
  }

  vector<string> ret(id_set.size());
  copy(id_set.begin(), id_set.end(), ret.begin());
  ids.swap(ret);
}

void lsh_index_storage::similar_row(
    const vector<float>& hash,
    double norm,
    uint64_t probe_num,
    uint64_t ret_num,
    vector<pair<string, double> >& ids) const {
  const vector<float> shifted = shift_hash(hash, shift_);
  const bit_vector bv = binarize(hash);

  lsh_probe_generator gen(shifted, table_num_);
  unordered_set<uint64_t> cands;

  for (uint64_t i = 0; i < table_num_; ++i) {
    lsh_vector key = gen.base(i);
    key.push_back(i);
    if (retrieve_hit_rows(hash_lv(key), ret_num, cands)) {
      get_sorted_similar_rows(cands, bv, norm, ret_num, ids);
      return;
    }
  }

  gen.init();
  for (uint64_t i = 0; i < probe_num; ++i) {
    pair<size_t, lsh_vector> p = gen.get_next_table_and_vector();
    p.second.push_back(p.first);
    if (retrieve_hit_rows(hash_lv(p.second), ret_num, cands)) {
      break;
    }
  }
  get_sorted_similar_rows(cands, bv, norm, ret_num, ids);
}

void lsh_index_storage::similar_row(
    const string& id,
    uint64_t ret_num,
    vector<pair<string, double> >& ids) const {
  lsh_master_table_t::const_iterator it = master_table_diff_.find(id);
  if (it == master_table_diff_.end()) {
    it = master_table_.find(id);
    if (it == master_table_.end()) {
      return;
    }
  }

  unordered_set<uint64_t> cands;
  for (size_t i = 0; i < it->second.lsh_hash.size(); ++i) {
    if (retrieve_hit_rows(it->second.lsh_hash[i], ret_num, cands)) {
      break;
    }
  }

  get_sorted_similar_rows(cands,
                          it->second.simhash_bv,
                          it->second.norm,
                          ret_num, ids);
}

string lsh_index_storage::name() const {
  return "lsh_index_storage";
}

void lsh_index_storage::pack(framework::packer& packer) const {
  packer.pack(*this);
}

void lsh_index_storage::unpack(msgpack::object o) {
  o.convert(this);
}

void lsh_index_storage::get_diff(lsh_master_table_t& diff) const {
  diff = master_table_diff_;
}

bool lsh_index_storage::put_diff(
    const lsh_master_table_t& diff) {
  for (lsh_master_table_t::const_iterator it = diff.begin(); it != diff.end();
      ++it) {
    if (it->second.lsh_hash.empty()) {
      // empty lsh_hash was propagated from other nodes. This indicates
      // that the row should be removed globally from the master table.
      if (unlearner_) {
        unlearner_->remove(it->first);
      }
      remove_model_row(it->first);
      master_table_.erase(it->first);
    } else {
      if (unlearner_) {
        if (unlearner_->can_touch(it->first)) {
          unlearner_->touch(it->first);
        } else {
          continue;  // drop untouchable value
        }
      }
      remove_model_row(it->first);
      set_mixed_row(it->first, it->second);
    }
  }

  // Find rows that were removed by unlearner or remove_row between
  // get_diff and put_diff
  std::vector<std::string> removed_rows;
  for (lsh_master_table_t::const_iterator it = master_table_diff_.begin();
      it != master_table_diff_.end(); ++it) {
    if (it->second.lsh_hash.empty()) {
      lsh_master_table_t::const_iterator pos;
      pos = diff.find(it->first);
      if (pos == diff.end() || !pos->second.lsh_hash.empty()) {
        removed_rows.push_back(it->first);
      }
    }
  }

  master_table_diff_.clear();

  // Keep empty rows in the diff table until next MIX to
  // propagate the removal of this data to other nodes.
  for (size_t i = 0; i < removed_rows.size(); ++i) {
    master_table_diff_.insert(make_pair(removed_rows[i], lsh_entry()));
    lsh_master_table_t::iterator it = master_table_.find(removed_rows[i]);
    if (it != master_table_.end()) {
      const uint64_t row_id = key_manager_.get_id_const(removed_rows[i]);
      lsh_entry& entry = it->second;
      put_empty_entry(row_id, entry);
    }
  }

  // lsh_table_diff_ is actually not MIXed, but must be cleared as well as diff
  // of usual model.
  lsh_table_diff_.clear();
  return true;
}

void lsh_index_storage::mix(
    const lsh_master_table_t& lhs,
    lsh_master_table_t& rhs) const {
  for (lsh_master_table_t::const_iterator it = lhs.begin(); it != lhs.end();
       ++it) {
    rhs[it->first] = it->second;
  }
}

// private

lsh_master_table_t::iterator lsh_index_storage::remove_and_get_row(
    const string& row) {
  const uint64_t row_id = key_manager_.get_id_const(row);
  if (row_id == common::key_manager::NOTFOUND) {
    return master_table_diff_.end();
  }

  lsh_master_table_t::iterator entry_it = master_table_diff_.find(row);
  lsh_master_table_t::iterator ret_it = entry_it;
  if (entry_it == master_table_diff_.end()) {
    ret_it = master_table_diff_.insert(make_pair(row, lsh_entry())).first;
    entry_it = master_table_.find(row);
    if (entry_it == master_table_.end()) {
      return ret_it;
    }
  }
  lsh_entry& entry = entry_it->second;
  put_empty_entry(row_id, entry);

  return ret_it;
}

void lsh_index_storage::put_empty_entry(
    uint64_t row_id,
    const lsh_entry& entry) {
  for (size_t i = 0; i < entry.lsh_hash.size(); ++i) {
    lsh_table_t::iterator it = lsh_table_diff_.find(entry.lsh_hash[i]);
    if (it != lsh_table_diff_.end()) {
      vector<uint64_t>& range = it->second;
      vector<uint64_t>::iterator jt = lower_bound(range.begin(),
                                                  range.end(),
                                                  row_id);
      if (jt != range.end() && row_id == *jt) {
        range.erase(jt);
        if (range.empty()) {
          lsh_table_diff_.erase(it);
        }
      }
    }
  }
}

vector<float> lsh_index_storage::make_entry(
    const vector<float>& hash,
    double norm,
    lsh_entry& entry) const {
  const vector<float> shifted = shift_hash(hash, shift_);
  lsh_probe_generator gen(shifted, table_num_);

  entry.lsh_hash.resize(table_num_);
  for (uint64_t i = 0; i < table_num_; ++i) {
    lsh_vector key = gen.base(i);
    key.push_back(i);
    entry.lsh_hash[i] = hash_lv(key);
  }

  entry.simhash_bv = binarize(hash);
  entry.norm = norm;

  return shifted;
}

// TODO(unknown): Separate implementation detail of processing
// lsh_table_ into another class
void lsh_index_storage::remove_model_row(const std::string& row) {
  const lsh_entry* entry = get_lsh_entry(row);
  if (!entry) {
    return;
  }

  const uint64_t row_id = key_manager_.get_id_const(row);
  for (size_t i = 0; i < entry->lsh_hash.size(); ++i) {
    lsh_table_t::iterator it = lsh_table_.find(entry->lsh_hash[i]);
    if (it != lsh_table_.end()) {
      vector<uint64_t>& range = it->second;
      vector<uint64_t>::iterator jt = find(range.begin(), range.end(), row_id);
      if (jt != range.end()) {
        range.erase(jt);
        if (range.empty()) {
          lsh_table_.erase(it);
        }
      }
    }
  }
}

void lsh_index_storage::set_mixed_row(
    const string& row,
    const lsh_entry& entry) {
  const uint64_t row_id = key_manager_.get_id(row);
  master_table_[row] = entry;
  for (size_t i = 0; i < entry.lsh_hash.size(); ++i) {
    lsh_table_[entry.lsh_hash[i]].push_back(row_id);
  }
}

bool lsh_index_storage::retrieve_hit_rows(
    uint64_t hash,
    size_t ret_num,
    unordered_set<uint64_t>& cands) const {
  retrieve_hit_rows_from_table(hash, lsh_table_diff_, cands);
  retrieve_hit_rows_from_table(hash, lsh_table_, cands);
  return cands.size() >= static_cast<uint64_t>(ret_num);
}

void lsh_index_storage::get_sorted_similar_rows(
    const unordered_set<uint64_t>& cands,
    const bit_vector& query_simhash,
    double query_norm,
    uint64_t ret_num,
    vector<pair<string, double> >& ids) const {
  // Avoid string copy as far as possible
  vector<pair<uint64_t, float> > scored;
  scored.reserve(cands.size());
  for (unordered_set<uint64_t>::const_iterator it = cands.begin();
      it != cands.end(); ++it) {
    const lsh_entry* entry = get_lsh_entry(key_manager_.get_key(*it));
    if (!entry || entry->lsh_hash.empty()) {
      continue;
    }
    const double dist = calc_euclidean_distance(*entry, query_simhash,
                                               query_norm);
    scored.push_back(make_pair(*it, -dist));
  }

  if (scored.size() <= ret_num) {
    sort(scored.begin(), scored.end(), greater_second());
  } else {
    partial_sort(scored.begin(),
                 scored.begin() + ret_num, scored.end(),
                 greater_second());
    scored.resize(ret_num);
  }

  ids.resize(scored.size());
  for (size_t i = 0; i < scored.size(); ++i) {
    ids[i].first = key_manager_.get_key(scored[i].first);
    ids[i].second = scored[i].second;
  }
}

const lsh_entry* lsh_index_storage::get_lsh_entry(const string& row) const {
  lsh_master_table_t::const_iterator it = master_table_diff_.find(row);
  if (it == master_table_diff_.end()) {
    it = master_table_.find(row);
    if (it == master_table_.end()) {
      return 0;
    }
  }
  return &it->second;
}

}  // namespace storage
}  // namespace core
}  // namespace jubatus
