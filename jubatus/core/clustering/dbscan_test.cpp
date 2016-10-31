// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2014 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#include <vector>
#include <string>

#include <gtest/gtest.h>

#include "jubatus/util/lang/scoped_ptr.h"
#include "jubatus/util/math/random.h"
#include "clustering.hpp"
#include "dbscan.hpp"
#include "../common/type.hpp"
#include "types.hpp"
#include "jubatus/util/lang/cast.h"
#include "../common/exception.hpp"

using jubatus::util::lang::lexical_cast;
using jubatus::util::lang::shared_ptr;

namespace jubatus {
namespace core {
namespace clustering {

class dbscan_test : public ::testing::Test {
 protected:
  static const double eps = 4.0;
  static const size_t min_core_point = 2;

  virtual void SetUp() {
    dbscan_.reset(new dbscan(eps, min_core_point));
  }

  virtual void TearDown() {
    dbscan_.reset();
  }

  jubatus::util::lang::scoped_ptr<dbscan> dbscan_;

  void do_batch() {
    common::sfv_t p1;
    p1.push_back(std::make_pair("x", 1));
    p1.push_back(std::make_pair("y", 1));
    weighted_point wp1;
    wp1.id = "0";
    wp1.weight = 1.0;
    wp1.data = p1;

    common::sfv_t p2;
    p2.push_back(std::make_pair("x", 2));
    p2.push_back(std::make_pair("y", 2));
    weighted_point wp2;
    wp2.id = "1";
    wp2.weight = 1.0;
    wp2.data = p2;

    common::sfv_t p3;
    p3.push_back(std::make_pair("x", 4));
    p3.push_back(std::make_pair("y", 4));
    weighted_point wp3;
    wp3.id = "2";
    wp3.weight = 1.0;
    wp3.data = p3;

    common::sfv_t p4;
    p4.push_back(std::make_pair("x", 5));
    p4.push_back(std::make_pair("y", 5));
    weighted_point wp4;
    wp4.id = "3";
    wp4.weight = 1.0;
    wp4.data = p4;

    common::sfv_t p5;
    p5.push_back(std::make_pair("x", 50));
    p5.push_back(std::make_pair("y", 50));
    weighted_point wp5;
    wp5.id = "4";
    wp5.weight = 1.0;
    wp5.data = p5;

    common::sfv_t p6;
    p6.push_back(std::make_pair("x", 51));
    p6.push_back(std::make_pair("y", 51));
    weighted_point wp6;
    wp6.id = "5";
    wp6.weight = 1.0;
    wp6.data = p6;

    common::sfv_t p7;
    p7.push_back(std::make_pair("x", 100));
    p7.push_back(std::make_pair("y", 100));
    weighted_point wp7;
    wp7.id = "6";
    wp7.weight = 1.0;
    wp7.data = p7;

    wplist points;
    points.push_back(wp1);
    points.push_back(wp2);
    points.push_back(wp3);
    points.push_back(wp4);
    points.push_back(wp5);
    points.push_back(wp6);
    points.push_back(wp7);

    dbscan_->batch(points);
  }

  void do_two_clustering() {
    dbscan_->set_eps(100.0);
    jubatus::util::math::random::mtrand r(0);
    int data_num = 50;
    wplist points;

    for (int i = 0; i < data_num; ++i) {
      common::sfv_t x, y;
      weighted_point wp;

      x.push_back(std::make_pair("a", 100 + r.next_gaussian() * 20));
      x.push_back(std::make_pair("b", 100 + r.next_gaussian() * 40));

      wp.id = lexical_cast<std::string>(2*i);
      wp.weight = 1.0;
      wp.data = x;

      points.push_back(wp);

      y.push_back(std::make_pair("c", -1000 - r.next_gaussian() * 20));
      y.push_back(std::make_pair("d", -1000 - r.next_gaussian() * 40));

      wp.id = lexical_cast<std::string>(2*i+1);
      wp.weight = 1.0;
      wp.data = y;

      points.push_back(wp);
    }
    dbscan_->batch(points);
  }
};

TEST_F(dbscan_test, batch) {
  do_batch();

  std::vector<int> point_states = dbscan_->get_point_states();
  std::vector<wplist> clusters = dbscan_->get_clusters();

  ASSERT_EQ(lexical_cast<size_t>(2), clusters.size());
  ASSERT_EQ(lexical_cast<size_t>(4), clusters[0].size());
  ASSERT_EQ(lexical_cast<size_t>(2), clusters[1].size());
}

TEST_F(dbscan_test, devide_two_cluster) {
  do_two_clustering();

  std::vector<int> point_states = dbscan_->get_point_states();
  std::cout << "poinst states:" << point_states.size() << " [";
  for (std::vector<int>::iterator it = point_states.begin();
       it != point_states.end(); ++it) {
    std::cout << *it << " ";
  }
  std::cout << "]" << std::endl;
  std::vector<wplist> clusters = dbscan_->get_clusters();
  for (std::vector<wplist>::iterator it = clusters.begin();
       it != clusters.end(); ++it) {
    std::cout << "[ ";
    for (wplist::iterator wit = (*it).begin(); wit != (*it).end(); ++wit) {
      std::cout << (*wit).id << " ";
    }
    std::cout << "]" << std::endl;
  }

  size_t cluster_num(2);
  size_t cluster1_num(50);
  size_t cluster2_num(50);
  ASSERT_EQ(cluster_num, clusters.size());
  ASSERT_EQ(cluster1_num, clusters[0].size());
  ASSERT_EQ(cluster2_num, clusters[1].size());
}


}  // namespace clustering
}  // namespace core
}  // namespace jubatus
