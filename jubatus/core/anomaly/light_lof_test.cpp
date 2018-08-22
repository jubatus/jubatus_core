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

#include <map>
#include <string>
#include <vector>
#include <utility>
#include <gtest/gtest.h>
#include "jubatus/util/lang/cast.h"
#include "jubatus/util/math/random.h"
#include "../common/jsonconfig.hpp"
#include "../nearest_neighbor/euclid_lsh.hpp"
#include "../nearest_neighbor/lsh.hpp"
#include "../nearest_neighbor/minhash.hpp"
#include "light_lof.hpp"

using std::pair;
using std::make_pair;
using std::string;
using std::vector;
using jubatus::util::lang::lexical_cast;
using jubatus::util::lang::shared_ptr;
using jubatus::core::nearest_neighbor::nearest_neighbor_base;

namespace jubatus {
namespace core {
namespace anomaly {
namespace {

typedef ::testing::Types<
  nearest_neighbor::lsh,
  nearest_neighbor::minhash,
  nearest_neighbor::euclid_lsh> nearest_neighbor_types;

const char* const ID = "my_id";
const uint32_t SEED = 1091;

vector<common::sfv_t> draw_2d_points_from_gaussian(
    size_t num_points,
    float x_mean,
    float y_mean,
    float x_deviation,
    float y_deviation,
    jubatus::util::math::random::mtrand& mtr) {
  vector<common::sfv_t> points(num_points);
  for (size_t i = 0; i < points.size(); ++i) {
    const double x = mtr.next_gaussian(x_mean, x_deviation);
    const double y = mtr.next_gaussian(y_mean, y_deviation);
    points[i].push_back(make_pair("x", x));
    points[i].push_back(make_pair("y", y));
  }
  return points;
}

common::sfv_t create_2d_point(double x, double y) {
  common::sfv_t point;
  point.push_back(make_pair("x", x));
  point.push_back(make_pair("y", y));
  return point;
}

}  // namespace

template<typename NearestNeighborMethod>
class light_lof_test : public ::testing::Test {
 public:
  static const int K = 10;
  void SetUp() {
    shared_ptr<storage::column_table> nn_table(new storage::column_table);
    light_lof::config config;
    config.nearest_neighbor_num = K;
    config.ignore_kth_same_point = ignore_kth_same_point();
    nn_engine_.reset(new NearestNeighborMethod(
        typename NearestNeighborMethod::config(), nn_table, ID));
    light_lof_.reset(new light_lof(config, ID, nn_engine_));

    mtr_ = jubatus::util::math::random::mtrand(SEED);
  }

 protected:
  shared_ptr<nearest_neighbor_base> nn_engine_;
  shared_ptr<light_lof> light_lof_;
  jubatus::util::math::random::mtrand mtr_;

 private:
  virtual bool ignore_kth_same_point() {
    return false;
  }
};

TYPED_TEST_CASE_P(light_lof_test);

template<typename NearestNeighborMethod>
class light_lof_with_ignore_kth_test
    : public light_lof_test<NearestNeighborMethod> {
 private:
  virtual bool ignore_kth_same_point() {
    return true;
  }
};

TYPED_TEST_CASE_P(light_lof_with_ignore_kth_test);

TYPED_TEST_P(light_lof_test, name) {
  EXPECT_EQ("light_lof", this->light_lof_->type());
}

TYPED_TEST_P(light_lof_test, get_all_row_ids) {
  const char* const raw_ids[] = { "id1", "id2", "id3" };
  const vector<string> ids(
      raw_ids, raw_ids + sizeof(raw_ids) / sizeof(raw_ids[0]));

  for (size_t i = 0; i < ids.size(); ++i) {
    EXPECT_TRUE(this->light_lof_->set_row(ids[i], common::sfv_t()));
  }
  vector<string> actual_ids;
  this->light_lof_->get_all_row_ids(actual_ids);
  EXPECT_EQ(ids, actual_ids);

  std::map<std::string, std::string> status;
  this->light_lof_->get_status(status);
  EXPECT_EQ(status["num_id"], lexical_cast<string>(actual_ids.size()));

  // duplicated set
  for (size_t i = 0; i < ids.size(); ++i) {
    EXPECT_TRUE(this->light_lof_->set_row(ids[i], common::sfv_t()));
  }
  this->light_lof_->get_all_row_ids(actual_ids);
  EXPECT_EQ(ids, actual_ids);

  status.clear();
  this->light_lof_->get_status(status);
  EXPECT_EQ(status["num_id"], lexical_cast<string>(actual_ids.size()));
}

TYPED_TEST_P(light_lof_test, calc_anomaly_score_on_gaussian_random_samples) {
  const vector<common::sfv_t> random_points =
      draw_2d_points_from_gaussian(90, 3, 1, 1, 0.5, this->mtr_);
  for (size_t i = 0; i < random_points.size(); ++i) {
    this->light_lof_->set_row(lexical_cast<string>(i), random_points[i]);
  }

  // Mean should be treated as normal.
  const common::sfv_t normal_query = create_2d_point(3, 1);
  EXPECT_GT(1.25, this->light_lof_->calc_anomaly_score(normal_query));

  // Outlier point should be treated as anomaly.
  const common::sfv_t outlier_query = create_2d_point(0, 3);
  EXPECT_LT(2.0, this->light_lof_->calc_anomaly_score(outlier_query));
}

TYPED_TEST_P(light_lof_with_ignore_kth_test, ignore_kth_same_point) {
  common::sfv_t dup1, dup2;
  dup1.push_back(make_pair("a", 1.0));
  dup1.push_back(make_pair("b", 2.0));
  dup2.push_back(make_pair("x", 1.0));
  dup2.push_back(make_pair("y", 2.0));

  this->light_lof_->clear();
  EXPECT_TRUE(this->light_lof_->set_row("dummy_1", dup1));
  EXPECT_TRUE(this->light_lof_->set_row("dummy_2", dup1));
  EXPECT_TRUE(this->light_lof_->set_row("dummy_3", dup1));

  std::map<std::string, std::string> status;
  this->light_lof_->get_status(status);
  EXPECT_EQ(status["num_ignored"], "0");

  for (int i = 0; i < (this->K - 1); ++i) {
    EXPECT_TRUE(this->light_lof_->set_row(
        lexical_cast<string>(i) + "th_point", dup2));
  }
  EXPECT_FALSE(this->light_lof_->set_row("Kth_point", dup2));

  status.clear();
  this->light_lof_->get_status(status);
  EXPECT_EQ(status["num_ignored"], "1");
}

TYPED_TEST_P(light_lof_test, config_validation) {
  light_lof::config c;

  c.reverse_nearest_neighbor_num = 10;

  // nearest_neighbor_num <= 2
  c.nearest_neighbor_num = 1;
  ASSERT_THROW(
      this->light_lof_.reset(new light_lof(c, ID, this->nn_engine_)),
      common::invalid_parameter);

  c.nearest_neighbor_num = 2;
  ASSERT_NO_THROW(
      this->light_lof_.reset(new light_lof(c, ID, this->nn_engine_)));

  c.nearest_neighbor_num = 3;
  ASSERT_NO_THROW(
      this->light_lof_.reset(new light_lof(c, ID, this->nn_engine_)));

  // nearest_neighbor_num <= reverse_nearest_neighbor_num
  c.reverse_nearest_neighbor_num = 2;
  ASSERT_THROW(
      this->light_lof_.reset(new light_lof(c, ID, this->nn_engine_)),
      common::invalid_parameter);

  c.reverse_nearest_neighbor_num = 3;
  ASSERT_NO_THROW
    (this->light_lof_.reset(new light_lof(c, ID, this->nn_engine_)));

  // ignore_kth_same_point is undefined
  c.ignore_kth_same_point = jubatus::util::data::optional<bool>();
  ASSERT_NO_THROW(
    this->light_lof_.reset(new light_lof(c, ID, this->nn_engine_)));
  EXPECT_TRUE(this->light_lof_->set_row("test", common::sfv_t()));
}

TYPED_TEST_P(light_lof_test, set_bulk) {
  vector<pair<string, common::sfv_t> > data;
  vector<string> ids;
  for (int i = 0; i < 10; i++) {
    common::sfv_t v;
    std::ostringstream id;
    id << i;
    v.push_back(make_pair("x", static_cast<double>(i)));
    data.push_back(make_pair(id.str(), v));
  }
  ids = this->light_lof_->set_bulk(data);
  EXPECT_EQ(ids.size(), 10);
}

TYPED_TEST_P(light_lof_test, bulk_ignore_kth) {
  light_lof::config c;
  c.ignore_kth_same_point = jubatus::util::data::optional<bool>(true);
  this->light_lof_.reset(new light_lof(c, ID, this->nn_engine_));
  vector<pair<string, common::sfv_t> > data;
  vector<string> ids;
  common::sfv_t v;
  for (int i = 0; i < 10; i++) {
    v.clear();
    std::ostringstream id;
    id << i;
    v.push_back(make_pair("x", 1.0));
    v.push_back(make_pair("y", 1.0));
    data.push_back(make_pair(id.str(), v));
  }
  ids = this->light_lof_->set_bulk(data);
  EXPECT_EQ(ids.size(), 9);
  v.clear();
  v.push_back(make_pair("x", 1.0));
  v.push_back(make_pair("y", -1.0));
  EXPECT_TRUE(this->light_lof_->set_row("10", v));
}



REGISTER_TYPED_TEST_CASE_P(
    light_lof_test,
    name,
    get_all_row_ids,
    calc_anomaly_score_on_gaussian_random_samples,
    config_validation,
    set_bulk,
    bulk_ignore_kth);

INSTANTIATE_TYPED_TEST_CASE_P(
    usual_case, light_lof_test, nearest_neighbor_types);

REGISTER_TYPED_TEST_CASE_P(
    light_lof_with_ignore_kth_test,
    ignore_kth_same_point);

INSTANTIATE_TYPED_TEST_CASE_P(
    ignore_kth_case, light_lof_with_ignore_kth_test, nearest_neighbor_types);

// TODO(beam2d): Add test of MIX.

}  // namespace anomaly
}  // namespace core
}  // namespace jubatus
