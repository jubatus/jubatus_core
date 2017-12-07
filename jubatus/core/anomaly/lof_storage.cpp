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

#include "lof_storage.hpp"

#include <algorithm>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "jubatus/util/lang/cast.h"

#include "anomaly_type.hpp"
#include "../common/exception.hpp"
#include "../common/jsonconfig.hpp"
#include "../common/vector_util.hpp"
#include "../recommender/euclid_lsh.hpp"
#include "../recommender/recommender_factory.hpp"

using jubatus::util::data::unordered_map;
using jubatus::util::data::unordered_set;
using jubatus::util::lang::shared_ptr;
using std::istream;
using std::istringstream;
using std::max;
using std::min;
using std::numeric_limits;
using std::ostream;
using std::ostringstream;
using std::pair;
using std::string;
using std::vector;

namespace jubatus {
namespace core {
namespace anomaly {

const uint32_t lof_storage::DEFAULT_NEIGHBOR_NUM = 10;
const uint32_t lof_storage::DEFAULT_REVERSE_NN_NUM = 30;
const bool lof_storage::DEFAULT_IGNORE_KTH_SAME_POINT = false;

lof_storage::config::config()
    : nearest_neighbor_num(DEFAULT_NEIGHBOR_NUM),
      reverse_nearest_neighbor_num(DEFAULT_REVERSE_NN_NUM),
      ignore_kth_same_point(DEFAULT_IGNORE_KTH_SAME_POINT) {
}

lof_storage::lof_storage()
    : neighbor_num_(DEFAULT_NEIGHBOR_NUM),
      reverse_nn_num_(DEFAULT_REVERSE_NN_NUM),
      ignore_kth_same_point_(DEFAULT_IGNORE_KTH_SAME_POINT),
      ignored_count_(0),
      nn_engine_(recommender::recommender_factory::create_recommender(
          "euclid_lsh",
          common::jsonconfig::config(jubatus::util::text::json::to_json(
              recommender::euclid_lsh::config())), "")) {
}

lof_storage::lof_storage(
    shared_ptr<recommender::recommender_base> nn_engine)
    : neighbor_num_(DEFAULT_NEIGHBOR_NUM),
      reverse_nn_num_(DEFAULT_REVERSE_NN_NUM),
      ignore_kth_same_point_(DEFAULT_IGNORE_KTH_SAME_POINT),
      ignored_count_(0),
      nn_engine_(nn_engine) {
}

lof_storage::lof_storage(
    const config& config,
    shared_ptr<recommender::recommender_base> nn_engine)
    : neighbor_num_(config.nearest_neighbor_num),
      reverse_nn_num_(config.reverse_nearest_neighbor_num),
      ignore_kth_same_point_(
          config.ignore_kth_same_point && *config.ignore_kth_same_point),
      ignored_count_(0),
      nn_engine_(nn_engine) {
}

lof_storage::~lof_storage() {
}

/**
 * Collect neighbor LRDs for the given query.
 */
float lof_storage::collect_lrds(
    const common::sfv_t& query,
    unordered_map<string, float>& neighbor_lrd) const {
  vector<pair<string, float> > neighbors;
  nn_engine_->neighbor_row(query, neighbors, neighbor_num_);

  return collect_lrds_from_neighbors(neighbors, neighbor_lrd);
}

/**
 * Collect neighbor LRDs for the given ID.
 * Note that returned `neighbor_lrd` does not contain the ID being queried.
 */
float lof_storage::collect_lrds(
    const string& id,
    unordered_map<string, float>& neighbor_lrd) const {
  vector<pair<string, float> > neighbors;
  nn_engine_->neighbor_row(id, neighbors, neighbor_num_ + 1);

  // neighbor_row returns id itself, so we remove it from the list
  for (size_t i = 0; i < neighbors.size(); ++i) {
    if (neighbors[i].first == id) {
      swap(neighbors[i], neighbors.back());
      neighbors.pop_back();
      break;
    }
  }

  return collect_lrds_from_neighbors(neighbors, neighbor_lrd);
}

float lof_storage::collect_lrds(
    const string& id,
    const common::sfv_t& query,
    jubatus::util::data::unordered_map<string, float>&
    neighbor_lrd) const {
  common::sfv_t updated_row;
  nn_engine_->decode_row(id, updated_row);
  common::merge_vector(updated_row, query);
  return collect_lrds(updated_row, neighbor_lrd);
}

void lof_storage::remove_row(const string& row) {
  mark_removed(lof_table_diff_[row]);
  nn_engine_->clear_row(row);
}

void lof_storage::clear() {
  lof_table_t().swap(lof_table_);
  lof_table_t().swap(lof_table_diff_);
  nn_engine_->clear();
  ignored_count_ = 0;
}

void lof_storage::get_all_row_ids(vector<string>& ids) const {
  nn_engine_->get_all_row_ids(ids);
}

void lof_storage::get_status(std::map<string, string>& status) const {
  status["num_id_master"] =
    jubatus::util::lang::lexical_cast<string>(lof_table_.size());
  status["num_id_diff"] =
    jubatus::util::lang::lexical_cast<string>(lof_table_diff_.size());

  if (ignore_kth_same_point_) {
    status["num_ignored"] =
      jubatus::util::lang::lexical_cast<string>(ignored_count_);
  }
}


vector<string> lof_storage::update_bulk(
    const vector<pair<string, common::sfv_t> > diff) {

  vector<pair<string, common::sfv_t> > update_data;
  unordered_set<string> update_set;
  vector<string> updated_ids;

  {
    vector<pair<string, common::sfv_t> >::const_iterator it;
    if (ignore_kth_same_point_) {
      vector<pair<string, common::sfv_t> > update_data;
      for (it = diff.begin(); it < diff.end(); ++it) {
        vector<pair<string, float> > nn_result;
        common::sfv_t updated_row;
        nn_engine_->decode_row((*it).first, updated_row);
        common::merge_vector(updated_row, (*it).second);

        nn_engine_->neighbor_row(
            updated_row, nn_result, neighbor_num_ - 1);
        if (nn_result.size() == (neighbor_num_ - 1) &&
            (nn_result.back().second == 0)) {
          ++ignored_count_;
        } else {
          {
            common::sfv_t query;
            nn_engine_->decode_row((*it).first, query);
            if (!query.empty()) {
              collect_neighbors((*it).first, update_set);
            }
          }
          // nn_engine_->update_row((*it).first, (*it).second);
          update_data.push_back(*it);
        }
      }
    } else {
      for (it = diff.begin(); it < diff.end(); ++it) {
        // nn_engine_->update_row((*it).first, (*it).second);
          {
            common::sfv_t query;
            nn_engine_->decode_row((*it).first, query);
            if (!query.empty()) {
              collect_neighbors((*it).first, update_set);
            }
          }
      }
      update_data = diff;
    }
  }

  {
    vector<pair<string, common::sfv_t> >::const_iterator it;
    for (it = update_data.begin(); it < update_data.end(); ++it) {
      nn_engine_->update_row((*it).first, (*it).second);
      collect_neighbors((*it).first, update_set);
      update_set.insert((*it).first);
      updated_ids.push_back((*it).first);
    }
  }

  update_entries(update_set);
  return updated_ids;
}

bool lof_storage::update_row(const string& row, const common::sfv_t& diff) {
  if (ignore_kth_same_point_) {
    vector<pair<string, float> > nn_result;

    // Find k-1 NNs for the given sfv.
    // If the distance to the (k-1) th neighbor is 0, the model already
    // have (k-1) points that have the same feature vector as given sfv.
    common::sfv_t updated_row;
    nn_engine_->decode_row(row, updated_row);
    common::merge_vector(updated_row, diff);

    nn_engine_->neighbor_row(
        updated_row, nn_result, neighbor_num_ - 1);
    if (nn_result.size() == (neighbor_num_ - 1) &&
       (nn_result.back().second == 0)) {
      ++ignored_count_;
      return false;
    }
  }

  unordered_set<string> update_set;

  {
    common::sfv_t query;
    nn_engine_->decode_row(row, query);
    if (!query.empty()) {
      collect_neighbors(row, update_set);
    }
  }

  nn_engine_->update_row(row, diff);
  collect_neighbors(row, update_set);

  update_set.insert(row);

  update_entries(update_set);
  return true;
}

string lof_storage::name() const {
  return "lof_storage";
}

float lof_storage::get_kdist(const string& row) const {
  lof_table_t::const_iterator it = lof_table_diff_.find(row);
  if (it == lof_table_diff_.end()) {
    it = lof_table_.find(row);
    if (it == lof_table_.end()) {
      throw JUBATUS_EXCEPTION(
        common::exception::runtime_error("specified row does not exist")
        << common::exception::error_message("row id: " + row));
    }
  } else if (is_removed(it->second)) {
    throw JUBATUS_EXCEPTION(
      common::exception::runtime_error("specified row is recently removed")
      << common::exception::error_message("row id: " + row));
  }
  return it->second.kdist;
}

float lof_storage::get_lrd(const string& row) const {
  lof_table_t::const_iterator it = lof_table_diff_.find(row);
  if (it == lof_table_diff_.end()) {
    it = lof_table_.find(row);
    if (it == lof_table_.end()) {
      throw JUBATUS_EXCEPTION(
        common::exception::runtime_error("specified row does not exist")
        << common::exception::error_message("row id: " + row));
    }
  } else if (is_removed(it->second)) {
    throw JUBATUS_EXCEPTION(
      common::exception::runtime_error("specified row is recently removed")
      << common::exception::error_message("row id: " + row));
  }
  return it->second.lrd;
}

bool lof_storage::has_row(const string& row) const {
  return lof_table_diff_.count(row) > 0 || lof_table_.count(row) > 0;
}

void lof_storage::update_all() {
  vector<string> ids;
  get_all_row_ids(ids);

  // NOTE: These two loops are separated, since update_lrd requires new kdist
  // values of k-NN.
  for (size_t i = 0; i < ids.size(); ++i) {
    update_kdist(ids[i]);
  }
  for (size_t i = 0; i < ids.size(); ++i) {
    update_lrd(ids[i]);
  }
}

void lof_storage::set_nn_engine(
    shared_ptr<recommender::recommender_base> nn_engine) {
  nn_engine_ = nn_engine;
}

void lof_storage::get_diff(lof_table_t& diff) const {
  diff = lof_table_diff_;
}

bool lof_storage::put_diff(const lof_table_t& mixed_diff) {
  for (lof_table_t::const_iterator it = mixed_diff.begin();
       it != mixed_diff.end(); ++it) {
    if (is_removed(it->second)) {
      if (unlearner_) {
        unlearner_->remove(it->first);
      }
      lof_table_.erase(it->first);
    } else {
      if (unlearner_) {
        if (unlearner_->can_touch(it->first)) {
          unlearner_->touch(it->first);
        } else {
          continue;  // drop untouchable value
        }
      }
      lof_table_[it->first] = it->second;
    }
  }

  // Create a set of removed (unlearned) rows since get_diff.
  unordered_set<string> removed_ids;
  for (lof_table_t::const_iterator it = lof_table_diff_.begin();
      it != lof_table_diff_.end(); ++it) {
    if (is_removed(it->second)) {
      // The row is locally marked as removed.  We should check if others
      // knows about it; if the diff does not contain the information that
      // the row is removed, the row is removed after `get_diff` (including
      // rows unlearned during `put_diff` (above code)).
      lof_table_t::const_iterator pos = mixed_diff.find(it->first);
      if (pos == mixed_diff.end() || !is_removed(pos->second)) {
        removed_ids.insert(it->first);
        std::cout << it->first << std::endl;
      }
    }
  }

  lof_table_diff_.clear();

  // Keep removed rows in the diff area until next MIX to
  // propagate the removal of this data to other nodes.
  for (unordered_set<string>::const_iterator it = removed_ids.begin();
      it != removed_ids.end(); ++it) {
    mark_removed(lof_table_diff_[*it]);
  }

  return true;
}

void lof_storage::mix(const lof_table_t& lhs, lof_table_t& rhs) const {
  for (lof_table_t::const_iterator it = lhs.begin(); it != lhs.end(); ++it) {
    if (is_removed(it->second)) {
      mark_removed(rhs[it->first]);
    } else {
      rhs.insert(*it);
    }
  }
}

// private

// static
void lof_storage::mark_removed(lof_entry& entry) {
  entry.kdist = -1;
}

// static
bool lof_storage::is_removed(const lof_entry& entry) {
  return entry.kdist < 0;
}

/**
 * Based on the given neighbors (list of ID and the distance to it),
 * get the LRDs for each neighbor points and return the calculated
 * LRD for the point.
 * `neighbors` donesn't have to be sorted.
 */
float lof_storage::collect_lrds_from_neighbors(
    const vector<pair<string, float> >& neighbors,
    unordered_map<string, float>& neighbor_lrd) const {
  if (neighbors.empty()) {
    return numeric_limits<float>::infinity();
  }

  // collect lrd values of the nearest neighbors
  neighbor_lrd.clear();
  for (size_t i = 0; i < neighbors.size(); ++i) {
    neighbor_lrd[neighbors[i].first] = get_lrd(neighbors[i].first);
  }

  // return lrd of the query
  float sum_reachability = 0;
  for (size_t i = 0; i < neighbors.size(); ++i) {
    sum_reachability += max(neighbors[i].second, get_kdist(neighbors[i].first));
  }

  // All the neighbors seem to have a same feature vector.
  if (sum_reachability == 0) {
    return numeric_limits<float>::infinity();
  }

  return neighbors.size() / sum_reachability;
}

/**
 * Collect neighbors for the given ID.
 */
void lof_storage::collect_neighbors(
    const string& row,
    unordered_set<string>& nn) const {
  vector<pair<string, float> > neighbors;
  nn_engine_->neighbor_row(row, neighbors, reverse_nn_num_);

  for (size_t i = 0; i < neighbors.size(); ++i) {
    nn.insert(neighbors[i].first);
  }
}

/**
 * Update kdist and LRD for given points and its neighbors.
 */
void lof_storage::update_entries(const unordered_set<string>& rows) {
  // NOTE: These two loops are separated, since update_lrd requires new kdist
  // values of k-NN.
  typedef unordered_map<string, vector<pair<string, float> > >
    rows_to_neighbors_type;

  rows_to_neighbors_type rows_to_neighbors;
  for (unordered_set<string>::const_iterator it = rows.begin();
       it != rows.end(); ++it) {
    nn_engine_->neighbor_row(*it, rows_to_neighbors[*it], neighbor_num_);
  }

  for (rows_to_neighbors_type::const_iterator it = rows_to_neighbors.begin();
       it != rows_to_neighbors.end(); ++it) {
    update_kdist_with_neighbors(it->first, it->second);
  }
  for (rows_to_neighbors_type::const_iterator it = rows_to_neighbors.begin();
       it != rows_to_neighbors.end(); ++it) {
    update_lrd_with_neighbors(it->first, it->second);
  }
}

/**
 * Update kdist for the row.
 */
void lof_storage::update_kdist(const string& row) {
  vector<pair<string, float> > neighbors;
  nn_engine_->neighbor_row(row, neighbors, neighbor_num_);
  update_kdist_with_neighbors(row, neighbors);
}

/**
 * Update kdist for the row using given NN search result (`neighbors`).
 * Note that this method expects `neighbors` to be sorted by score.
 */
void lof_storage::update_kdist_with_neighbors(
    const string& row,
    const vector<pair<string, float> >& neighbors) {
  if (!neighbors.empty()) {
    lof_table_diff_[row].kdist = neighbors.back().second;
  }
}

/**
 * Update LRD for the row.
 */
void lof_storage::update_lrd(const string& row) {
  vector<pair<string, float> > neighbors;
  nn_engine_->neighbor_row(row, neighbors, neighbor_num_);
  update_lrd_with_neighbors(row, neighbors);
}

/**
 * Update LRD for the row using given NN search result (`neighbors`).
 */
void lof_storage::update_lrd_with_neighbors(
    const string& row, const vector<pair<string, float> >& neighbors) {
  if (neighbors.empty()) {
    lof_table_diff_[row].lrd = 1;
    return;
  }

  const size_t length = min(neighbors.size(), size_t(neighbor_num_));
  float sum_reachability = 0;
  for (size_t i = 0; i < length; ++i) {
    sum_reachability += max(neighbors[i].second, get_kdist(neighbors[i].first));
  }

  // NOTE : `sum_reachability` can be zero due to a numerical error,
  // which will cause the lof score inf.
  // To avoid inf score, a small number 1e-10 is added to `sum_reachability`.
  lof_table_diff_[row].lrd = length / (sum_reachability + 1e-10f);
}

}  // namespace anomaly
}  // namespace core
}  // namespace jubatus
