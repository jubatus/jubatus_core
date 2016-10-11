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

#include <cmath>
#include <string>
#include <utility>
#include <vector>
#include <gtest/gtest.h>
#include "inverted_index_storage.hpp"
#include "../framework/stream_writer.hpp"
#include "jubatus/util/lang/cast.h"

using std::make_pair;
using std::pair;
using std::string;
using std::stringstream;
using std::sqrt;
using std::vector;
using jubatus::util::lang::lexical_cast;

namespace jubatus {
namespace core {
namespace storage {

TEST(inverted_index_storage, trivial) {
  inverted_index_storage s;
  // r1: (1, 1, 1, 0, 0)
  s.set("c1", "r1", 1);
  s.set("c2", "r1", 1);
  s.set("c3", "r1", 1);
  // r2: (1, 0, 1, 1, 0)
  s.set("c1", "r2", 1);
  s.set("c3", "r2", 1);
  s.set("c4", "r2", 1);
  // r3: (0, 1, 0, 0, 1)
  s.set("c2", "r3", 1);
  s.set("c5", "r3", 1);

  // v:  (1, 1, 0, 0, 0)
  common::sfv_t v;
  v.push_back(make_pair("c1", 1.0));
  v.push_back(make_pair("c2", 1.0));

  vector<pair<string, float> > scores;
  s.calc_scores(v, scores, 100);

  ASSERT_EQ(3u, scores.size());
  EXPECT_FLOAT_EQ(2.0 / std::sqrt(3) / std::sqrt(2), scores[0].second);
  EXPECT_EQ("r1", scores[0].first);
  EXPECT_FLOAT_EQ(1.0 / std::sqrt(2) / std::sqrt(2), scores[1].second);
  EXPECT_EQ("r3", scores[1].first);
  EXPECT_FLOAT_EQ(1.0 / std::sqrt(2) / std::sqrt(3), scores[2].second);
  EXPECT_EQ("r2", scores[2].first);

  msgpack::sbuffer buf;
  framework::stream_writer<msgpack::sbuffer> st(buf);
  framework::jubatus_packer jp(st);
  framework::packer packer(jp);
  s.pack(packer);
  inverted_index_storage s2;
  msgpack::unpacked unpacked;
  msgpack::unpack(&unpacked, buf.data(), buf.size());
  s2.unpack(unpacked.get());
  vector<pair<string, float> > scores2;
  s.calc_scores(v, scores2, 100);
  // expect to get same result
  ASSERT_EQ(3u, scores2.size());
  EXPECT_FLOAT_EQ(2.0 / std::sqrt(3) / std::sqrt(2), scores2[0].second);
  EXPECT_EQ("r1", scores2[0].first);
  EXPECT_FLOAT_EQ(1.0 / std::sqrt(2) / std::sqrt(2), scores2[1].second);
  EXPECT_EQ("r3", scores2[1].first);
  EXPECT_FLOAT_EQ(1.0 / std::sqrt(2) / std::sqrt(3), scores2[2].second);
  EXPECT_EQ("r2", scores2[2].first);
}

TEST(inverted_index_storage, trivial_euclid) {
  inverted_index_storage s;
  // r1: (1, 1, 1, 0, 0)
  s.set("c1", "r1", 1);
  s.set("c2", "r1", 1);
  s.set("c3", "r1", 1);
  // r2: (1, 0, 1, 1, 0)
  s.set("c1", "r2", 1);
  s.set("c3", "r2", 1);
  s.set("c4", "r2", 1);
  // r3: (0, 1, 0, 0, 1)
  s.set("c2", "r3", 1);
  s.set("c5", "r3", 1);

  // v:  (1, 1, 0, 0, 0)
  common::sfv_t v;
  v.push_back(make_pair("c1", 1.0));
  v.push_back(make_pair("c2", 1.0));

  vector<pair<string, float> > scores;
  s.calc_euclid_scores(v, scores, 100);

  ASSERT_EQ(3, scores.size());
  EXPECT_EQ("r1", scores[0].first);
  EXPECT_FLOAT_EQ(-1, scores[0].second);
  EXPECT_EQ("r3", scores[1].first);
  EXPECT_FLOAT_EQ(- sqrt(2), scores[1].second);
  EXPECT_EQ("r2", scores[2].first);
  EXPECT_FLOAT_EQ(- sqrt(3), scores[2].second);

  // remove column "r3"
  s.remove("c2", "r3");
  s.remove("c5", "r3");

  scores.clear();
  s.calc_euclid_scores(v, scores, 100);
  ASSERT_EQ(2, scores.size());
  EXPECT_EQ("r1", scores[0].first);
  EXPECT_EQ("r2", scores[1].first);
}

TEST(inverted_index_storage, diff) {
  inverted_index_storage s;
  // r1: (1, 1, 0, 0, 0)
  s.set("c1", "r1", 1);
  s.set("c2", "r1", 1);

  inverted_index_storage::diff_type diff;
  s.get_diff(diff);

  inverted_index_storage t;
  t.put_diff(diff);
  EXPECT_EQ(1.0, t.get("c1", "r1"));
  EXPECT_EQ(1.0, t.get("c2", "r1"));
  EXPECT_EQ(0.0, t.get("c3", "r1"));
  EXPECT_EQ(0.0, t.get("c1", "r2"));
}

TEST(inverted_index_storage, column_operations) {
  std::vector<std::string> ids;
  inverted_index_storage s1;

  s1.set("c1", "r1", 1);
  s1.set("c1", "r2", 1);
  s1.set("c1", "r3", 1);
  s1.get_all_column_ids(ids);
  EXPECT_EQ(3u, ids.size());

  s1.remove("c1", "r1");
  s1.mark_column_removed("r1");
  s1.get_all_column_ids(ids);
  EXPECT_EQ(2u, ids.size());

  // do MIX
  inverted_index_storage::diff_type d1;
  s1.get_diff(d1);
  s1.put_diff(d1);

  s1.get_all_column_ids(ids);
  EXPECT_EQ(2u, ids.size());

  // Once MIXed, removing column does not take affect
  // until next MIX.
  s1.remove("c1", "r2");
  s1.get_all_column_ids(ids);
  EXPECT_EQ(2u, ids.size());

  // do MIX
  inverted_index_storage::diff_type d2;
  s1.get_diff(d2);
  s1.put_diff(d2);

  s1.get_all_column_ids(ids);
  ASSERT_EQ(1u, ids.size());
  EXPECT_EQ("r3", ids[0]);
}

TEST(inverted_index_storage, mix) {
  inverted_index_storage s1;
  // c1: (1, 1, 0)
  s1.set("c1", "r1", 1);
  s1.set("c1", "r2", 1);
  // c2: (0, 0, 1)
  s1.set("c2", "r3", 1);

  inverted_index_storage s2;
  // c1: (0, 2, 0)
  s2.set("c1", "r2", 2);

  inverted_index_storage::diff_type d1, d2;
  s1.get_diff(d1);
  s2.get_diff(d2);

  // d2 --> d1
  s2.mix(d2, d1);

  s2.put_diff(d1);

  // expected:
  //  c1: (1, 2, 0)   = (1,1,0) << (0,2,0)
  //  c2: (0, 0, 1)   = () << (0,0,1)
  EXPECT_EQ(1.0, s2.get("c1", "r1"));
  EXPECT_EQ(2.0, s2.get("c1", "r2"));
  EXPECT_EQ(0.0, s2.get("c1", "r3"));

  EXPECT_EQ(0.0, s2.get("c2", "r1"));
  EXPECT_EQ(0.0, s2.get("c2", "r2"));
  EXPECT_EQ(1.0, s2.get("c2", "r3"));
}

TEST(inverted_index_storage, mix_removal) {
  std::vector<std::string> ids;

  // setup s1
  inverted_index_storage s1;
  s1.set("c1", "r1", 0.000001);
  for (size_t i = 100; i < 1024; ++i) {
    s1.set("c" + lexical_cast<std::string>(i), "r2", i);
  }
  s1.set("c2", "r3", 0.0001);

  // setup s2
  inverted_index_storage s2;
  for (size_t i = 100; i < 1024; ++i) {
    s1.set("c" + lexical_cast<std::string>(i), "r2", i);
  }

  // setup s3
  inverted_index_storage s3;
  s3.set("c1", "r4", 0.0001);

  // MIX
  inverted_index_storage::diff_type d1, d2, d3;
  s1.get_diff(d1);
  s2.get_diff(d2);
  s3.get_diff(d3);
  s1.mix(d3, d2);  // d3 --> d2
  s1.mix(d2, d1);  // d2 --> d1
  s1.put_diff(d1);
  s2.put_diff(d1);
  s3.put_diff(d1);

  // rows must be registered in both storage
  s1.get_all_column_ids(ids);
  EXPECT_EQ(4, ids.size());  // r1, r2, r3, r4
  s2.get_all_column_ids(ids);
  EXPECT_EQ(4, ids.size());  // r1, r2, r3, r4
  s3.get_all_column_ids(ids);
  EXPECT_EQ(4, ids.size());  // r1, r2, r3, r4

  // remove column ``r2``
  for (size_t i = 100; i < 1024; ++i) {
    s1.remove("c" + lexical_cast<std::string>(i), "r2");
    s2.remove("c" + lexical_cast<std::string>(i), "r2");
  }
  s1.mark_column_removed("r2");
  s2.mark_column_removed("r2");

  // rows must not be removed from s1 before MIX
  s1.get_all_column_ids(ids);
  EXPECT_EQ(4, ids.size());  // r1, r2, r3, r4
  s2.get_all_column_ids(ids);
  EXPECT_EQ(4, ids.size());  // r1, r2, r3, r4

  // MIX
  s1.get_diff(d1);
  s2.get_diff(d2);
  s3.get_diff(d3);
  s1.mix(d3, d2);  // d3 --> d2
  s1.mix(d2, d1);  // d2 --> d1
  s1.put_diff(d1);
  s2.put_diff(d1);
  s3.put_diff(d1);

  // rows must be removed from both storage after MIX
  s1.get_all_column_ids(ids);
  EXPECT_EQ(3, ids.size());  // r1, r3, r4
  s2.get_all_column_ids(ids);
  EXPECT_EQ(3, ids.size());  // r1, r3, r4
  s3.get_all_column_ids(ids);
  EXPECT_EQ(3, ids.size());  // r1, r3, r4
}

TEST(inverted_index_storage, empty) {
  // v:  (1, 1, 0, 0, 0)
  common::sfv_t v;
  v.push_back(make_pair("c1", 1.0));
  v.push_back(make_pair("c2", 1.0));

  inverted_index_storage s;

  vector<pair<string, float> > scores;
  s.calc_scores(v, scores, 100);

  EXPECT_EQ(0u, scores.size());
}

}  // namespace storage
}  // namespace core
}  // namespace jubatus
