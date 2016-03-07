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

#include "inverted_index_storage.hpp"
#include <algorithm>
#include <cmath>
#include <functional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "../storage/fixed_size_heap.hpp"
#include "jubatus/util/data/unordered_map.h"

using std::istringstream;
using std::make_pair;
using std::ostringstream;
using std::pair;
using std::sort;
using std::sqrt;
using std::string;
using std::vector;
using jubatus::util::data::unordered_map;

namespace jubatus {
namespace core {
namespace storage {

inverted_index_storage::inverted_index_storage() {
}

inverted_index_storage::~inverted_index_storage() {
}

void inverted_index_storage::set(
    const std::string& row,
    const std::string& column,
    float val) {
  uint64_t column_id = column2id_.get_id_const(column);

  if (column_id == common::key_manager::NOTFOUND) {
    column_id = column2id_.get_id(column);
  } else {
    float cur_val = get(row, column);
    column2norm_diff_[column_id] -= cur_val * cur_val;
  }
  inv_diff_[row][column_id] = val;
  column2norm_diff_[column_id] += val * val;
  if (column2norm_diff_[column_id] == 0) {
    column2norm_diff_.erase(column_id);
  }
}

float inverted_index_storage::get(
    const string& row,
    const string& column) const {
  uint64_t column_id = column2id_.get_id_const(column);
  if (column_id == common::key_manager::NOTFOUND) {
    return 0.f;
  }
  {
    bool exist = false;
    float ret = get_from_tbl(row, column_id, inv_diff_, exist);
    if (exist) {
      return ret;
    }
  }
  {
    bool exist = false;
    float ret = get_from_tbl(row, column_id, inv_, exist);
    if (exist) {
      return ret;
    }
  }
  return 0.0;
}

float inverted_index_storage::get_from_tbl(
    const std::string& row,
    uint64_t column_id,
    const tbl_t& tbl,
    bool& exist) const {
  exist = false;

  if (column_id == common::key_manager::NOTFOUND) {
    return 0.f;
  }
  tbl_t::const_iterator it = tbl.find(row);
  if (it == tbl.end()) {
    return 0.f;
  } else {
    row_t::const_iterator it_row = it->second.find(column_id);
    if (it_row == it->second.end()) {
      return 0.f;
    } else {
      exist = true;
      return it_row->second;
    }
  }
}

void inverted_index_storage::remove(
    const std::string& row,
    const std::string& column) {
  uint64_t column_id = column2id_.get_id_const(column);
  if (column_id == common::key_manager::NOTFOUND) {
    return;
  }

  set(row, column, 0.f);

  // Test if the data exists in the master table.
  bool exist = false;
  get_from_tbl(row, column_id, inv_, exist);

  // If the data exists in the master table, we should
  // keep it in the diff table until next MIX to propagate
  // the removal of this data to other nodes.
  // Otherwise we can immediately remove the row from
  // the diff table.
  if (!exist) {
    tbl_t::iterator it = inv_diff_.find(row);
    if (it != inv_diff_.end()) {
      row_t::iterator it_row = it->second.find(column_id);
      if (it_row != it->second.end()) {
        it->second.erase(it_row);
        if (it->second.empty()) {
          // There are no columns that belongs to this row,
          // so we can remove the row itself.
          inv_diff_.erase(it);
        }
      }
    }
  }
}

void inverted_index_storage::clear() {
  tbl_t().swap(inv_);
  tbl_t().swap(inv_diff_);
  imap_float_t().swap(column2norm_);
  imap_float_t().swap(column2norm_diff_);
  common::key_manager().swap(column2id_);
}

void inverted_index_storage::get_all_column_ids(
    std::vector<std::string>& ids) const {
  ids.clear();
  for (imap_float_t::const_iterator it = column2norm_.begin();
      it != column2norm_.end(); ++it) {
    ids.push_back(column2id_.get_key(it->first));
  }
  for (imap_float_t::const_iterator it = column2norm_diff_.begin();
      it != column2norm_diff_.end(); ++it) {
    if (column2norm_.find(it->first) == column2norm_.end()) {
      if (0.f < it->second) {  // recently removed row has zero value
        ids.push_back(column2id_.get_key(it->first));
      } else {
        // remove if diff specify 0.0
        // column2norm_ may have recently removed data, we must exclude it
        for (std::vector<std::string>::iterator target = ids.begin();
             target != ids.end();
             ++target) {
          if (*target == column2id_.get_key(it->first)) {
            ids.erase(target);
            break;
          }
        }
      }
    }
  }
}

void inverted_index_storage::get_diff(diff_type& diff) const {
  for (tbl_t::const_iterator it = inv_diff_.begin(); it != inv_diff_.end();
      ++it) {
    vector<pair<string, float> > columns;
    for (row_t::const_iterator it2 = it->second.begin();
        it2 != it->second.end(); ++it2) {
      columns.push_back(make_pair(column2id_.get_key(it2->first), it2->second));
    }
    diff.inv.set_row(it->first, columns);
  }

  for (imap_float_t::const_iterator it = column2norm_diff_.begin();
      it != column2norm_diff_.end(); ++it) {
    diff.column2norm[column2id_.get_key(it->first)] = it->second;
  }
}

bool inverted_index_storage::put_diff(
    const diff_type& mixed_diff) {
  vector<string> ids;
  mixed_diff.inv.get_all_row_ids(ids);

  unordered_map<string, bool> update_list;  // bool:true->update, false->erase
  if (unlearner_) {
    for (size_t i = 0; i < ids.size(); ++i) {
      const string& row = ids[i];
      vector<pair<string, float> > columns;
      mixed_diff.inv.get_row(row, columns);
      for (size_t j = 0; j < columns.size(); ++j) {
        if (unlearner_->can_touch(columns[j].first)) {
          unordered_map<string, bool>::iterator it =
              update_list.find(columns[j].first);
          if (it == update_list.end()) {
            update_list.insert(
                make_pair(columns[j].first, 0 < columns[j].second));
          } else {
            // at least one value is not zero, it is update, not erase
            it->second = it->second || 0 < columns[j].second;
          }
        }
        // drop untouchable value
      }
    }

    for (unordered_map<string, bool>::const_iterator it = update_list.begin();
         it != update_list.end();
         ++it) {
      if (it->second) {  // update
        unlearner_->touch(it->first);
      } else {  // remove
        unlearner_->remove(it->first);
      }
    }
    for (unordered_map<string, bool>::iterator it = update_list.begin();
         it != update_list.end();
         ++it) {
      if (!unlearner_->exists_in_memory(it->first)) {
        it->second = false;  // unlearn
      }
    }
  }

  for (size_t i = 0; i < ids.size(); ++i) {
    const string& row = ids[i];
    row_t& v = inv_[row];
    vector<pair<string, float> > columns;
    mixed_diff.inv.get_row(row, columns);

    for (size_t j = 0; j < columns.size(); ++j) {
      size_t id = column2id_.get_id(columns[j].first);
      if (columns[j].second == 0.f) {
        v.erase(id);
      } else {
        if (unlearner_) {
          if (unlearner_->exists_in_memory(columns[j].first)) {
            v[id] = columns[j].second;
          }
          // drop data whichi unlearner could not touch
        } else {
          v[id] = columns[j].second;
        }
      }
    }
  }
  inv_diff_.clear();

  if (unlearner_) {
    for (map_float_t::const_iterator it = mixed_diff.column2norm.begin();
         it != mixed_diff.column2norm.end(); ++it) {
      unordered_map<string, bool>::const_iterator target =
          update_list.find(it->first);
      uint64_t column_index = column2id_.get_id(it->first);
      if (target != update_list.end() && target->second) {
        // unlearner admit to update
        column2norm_[column_index] += it->second;
        if (column2norm_[column_index] == 0.f) {
          column2norm_.erase(column_index);
        }
      } else {
        column2norm_.erase(column_index);
      }
    }
  } else {
    for (map_float_t::const_iterator it = mixed_diff.column2norm.begin();
         it != mixed_diff.column2norm.end(); ++it) {
      uint64_t column_index = column2id_.get_id(it->first);
      column2norm_[column_index] += it->second;
      if (column2norm_[column_index] == 0.f) {
        column2norm_.erase(column_index);
      }
    }
  }
  column2norm_diff_.clear();
  return true;
}

void inverted_index_storage::mix(const diff_type& lhs, diff_type& rhs) const {
  // merge inv diffs
  vector<string> ids;
  lhs.inv.get_all_row_ids(ids);
  for (size_t i = 0; i < ids.size(); ++i) {
    const string& row = ids[i];

    vector<pair<string, float> > columns;
    lhs.inv.get_row(row, columns);
    rhs.inv.set_row(row, columns);
  }

  // merge norm diffs
  for (map_float_t::const_iterator it = lhs.column2norm.begin();
      it != lhs.column2norm.end(); ++it) {
    rhs.column2norm[it->first] += it->second;
  }
}

void inverted_index_storage::pack(framework::packer& packer) const {
  packer.pack(*this);
}

void inverted_index_storage::unpack(msgpack::object o) {
  o.convert(this);
}

void inverted_index_storage::calc_scores(
    const common::sfv_t& query,
    vector<pair<string, float> >& scores,
    size_t ret_num) const {
  float query_norm = calc_l2norm(query);
  if (query_norm == 0.f) {
    return;
  }

  std::vector<float> i_scores(column2id_.get_max_id() + 1, 0.0);
  for (size_t i = 0; i < query.size(); ++i) {
    const string& fid = query[i].first;
    float val = query[i].second;
    add_inp_scores(fid, val, i_scores);
  }

  storage::fixed_size_heap<pair<float, uint64_t>,
                           std::greater<pair<float, uint64_t> > > heap(ret_num);
  for (size_t i = 0; i < i_scores.size(); ++i) {
    float score = i_scores[i];
    if (score == 0.f)
      continue;
    float norm = calc_columnl2norm(i);
    if (norm == 0.f)
      continue;
    float normed_score = score / norm / query_norm;
    heap.push(make_pair(normed_score, i));
  }
  vector<pair<float, uint64_t> > sorted_scores;
  heap.get_sorted(sorted_scores);

  for (size_t i = 0; i < sorted_scores.size() && i < ret_num; ++i) {
    scores.push_back(
        make_pair(column2id_.get_key(sorted_scores[i].second),
                  sorted_scores[i].first));
  }
}

/**
 * Returns the nearest neighbors measured by euclidean distance.
 * Scores are represented in sign-inverted euclidean distance (<= 0);
 * i.e., larger score means high similarity.
 */
void inverted_index_storage::calc_euclid_scores(
    const common::sfv_t& query,
    vector<pair<string, float> >& scores,
    size_t ret_num) const {
  std::vector<float> i_scores(column2id_.get_max_id() + 1, 0.0);
  for (size_t i = 0; i < query.size(); ++i) {
    const string& fid = query[i].first;
    float val = query[i].second;
    add_inp_scores(fid, val, i_scores);
  }

  storage::fixed_size_heap<pair<float, uint64_t>,
                           std::greater<pair<float, uint64_t> > > heap(ret_num);

  float query_norm = calc_l2norm(query);
  for (size_t i = 0; i < i_scores.size(); ++i) {
    float norm = calc_columnl2norm(i);

    if (norm == 0.f) {
      // The column is already removed.
      continue;
    }

    // `d2` is a squared euclidean distance.
    // In edgy cases, `d2` may sliglty become negative (which is actually
    // expected to be 0) due to the floating point precision problem.
    // This cause `sqrt(d2)` to return NaN, which is not what we want.
    // To avoid this we use `sqrt(max(0, d2))`.
    float d2 = query_norm * query_norm + norm * norm - 2 * i_scores[i];
    heap.push(make_pair(-std::sqrt(std::max(0.0f, d2)), i));
  }

  vector<pair<float, uint64_t> > sorted_scores;
  heap.get_sorted(sorted_scores);

  for (size_t i = 0; i < sorted_scores.size() && i < ret_num; ++i) {
    scores.push_back(
        make_pair(column2id_.get_key(sorted_scores[i].second),
                  sorted_scores[i].first));
  }
}

float inverted_index_storage::calc_l2norm(const common::sfv_t& sfv) {
  float ret = 0.f;
  for (size_t i = 0; i < sfv.size(); ++i) {
    ret += sfv[i].second * sfv[i].second;
  }
  return std::sqrt(ret);
}

float inverted_index_storage::calc_columnl2norm(uint64_t column_id) const {
  float ret = 0.f;
  imap_float_t::const_iterator it_diff = column2norm_diff_.find(column_id);
  if (it_diff != column2norm_diff_.end()) {
    ret += it_diff->second;
  }
  imap_float_t::const_iterator it = column2norm_.find(column_id);
  if (it != column2norm_.end()) {
    ret += it->second;
  }
  return std::sqrt(ret);
}

void inverted_index_storage::add_inp_scores(
    const std::string& row,
    float val,
    std::vector<float>& scores) const {
  tbl_t::const_iterator it_diff = inv_diff_.find(row);
  if (it_diff != inv_diff_.end()) {
    const row_t& row_v = it_diff->second;
    for (row_t::const_iterator row_it = row_v.begin(); row_it != row_v.end();
        ++row_it) {
      scores[row_it->first] += row_it->second * val;
    }
  }

  tbl_t::const_iterator it = inv_.find(row);
  if (it != inv_.end()) {
    const row_t& row_v = it->second;
    if (it_diff == inv_diff_.end()) {
      for (row_t::const_iterator row_it = row_v.begin(); row_it != row_v.end();
          ++row_it) {
        scores[row_it->first] += row_it->second * val;
      }
    } else {
      const row_t& row_diff_v = it_diff->second;
      for (row_t::const_iterator row_it = row_v.begin(); row_it != row_v.end();
          ++row_it) {
        if (row_diff_v.find(row_it->first) == row_diff_v.end()) {
          scores[row_it->first] += row_it->second * val;
        }
      }
    }
  }
}

std::string inverted_index_storage::name() const {
  return string("inverted_index_storage");
}

}  // namespace storage
}  // namespace core
}  // namespace jubatus
