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

#include <string>
#include <vector>
#include <gtest/gtest.h>
#include <msgpack.hpp>

#include "../common/exception.hpp"
#include "../framework/packer.hpp"
#include "../framework/stream_writer.hpp"
#include "lru_unlearner.hpp"
#include "test_util.hpp"

namespace jubatus {
namespace core {
namespace unlearner {

TEST(lru_unlearner, max_size_must_be_positive) {
  lru_unlearner::config config;
  config.max_size = -1;
  EXPECT_THROW(lru_unlearner unlearner(config), common::config_exception);
  config.max_size = 0;
  EXPECT_THROW(lru_unlearner unlearner(config), common::config_exception);
}

TEST(lru_unlearner, trivial) {
  lru_unlearner::config config;
  config.max_size = 3;
  lru_unlearner unlearner(config);

  mock_callback callback;
  unlearner.set_callback(callback);

  std::vector<std::string> touch_sequence;
  touch_sequence.push_back("id1");
  touch_sequence.push_back("id2");
  touch_sequence.push_back("id3");
  touch_sequence.push_back("id2");
  touch_sequence.push_back("id1");

  for (size_t i = 0; i < touch_sequence.size(); ++i) {
    unlearner.touch(touch_sequence[i]);
    EXPECT_EQ("", callback.unlearned_id());
    EXPECT_TRUE(unlearner.exists_in_memory(touch_sequence[i]));
  }

  unlearner.touch("id4");
  EXPECT_EQ("id3", callback.unlearned_id());
  EXPECT_FALSE(unlearner.exists_in_memory(callback.unlearned_id()));
}

TEST(lru_unlearner, sticky) {
  lru_unlearner::config config;
  config.max_size = 3;
  config.sticky_pattern = "sticky-*";
  lru_unlearner unlearner(config);

  mock_callback callback;
  unlearner.set_callback(callback);

  std::vector<std::string> touch_sequence;
  touch_sequence.push_back("id1");
  touch_sequence.push_back("id2");
  touch_sequence.push_back("sticky-1");

  for (size_t i = 0; i < touch_sequence.size(); ++i) {
    unlearner.touch(touch_sequence[i]);
    EXPECT_EQ("", callback.unlearned_id());
    EXPECT_TRUE(unlearner.exists_in_memory(touch_sequence[i]));
  }

  unlearner.touch("id3");
  EXPECT_EQ("id1", callback.unlearned_id());
  EXPECT_FALSE(unlearner.exists_in_memory(callback.unlearned_id()));

  unlearner.touch("id4");
  EXPECT_EQ("id2", callback.unlearned_id());

  unlearner.touch("id5");
  EXPECT_EQ("id3", callback.unlearned_id());

  unlearner.touch("sticky-2");
  EXPECT_EQ("id4", callback.unlearned_id());

  unlearner.touch("sticky-3");
  EXPECT_EQ("id5", callback.unlearned_id());

  EXPECT_FALSE(unlearner.touch("id6"));

  unlearner.remove("sticky-1");
  EXPECT_FALSE(unlearner.exists_in_memory("sticky-1"));

  EXPECT_TRUE(unlearner.touch("id6"));
}

TEST(lru_unlearner, pack_and_unpack) {
  lru_unlearner::config config;
  config.max_size = 3;
  config.sticky_pattern = "sticky-*";
  lru_unlearner unlearner_1(config);
  lru_unlearner unlearner_2(config);

  mock_callback callback_1, callback_2;
  unlearner_1.set_callback(callback_1);
  unlearner_2.set_callback(callback_2);

  std::vector<std::string> touch_sequence;
  touch_sequence.push_back("id1");
  touch_sequence.push_back("id2");
  touch_sequence.push_back("sticky-1");

  for (size_t i = 0; i < touch_sequence.size(); ++i) {
    unlearner_1.touch(touch_sequence[i]);
  }

  msgpack::sbuffer buf;
  {
    framework::stream_writer<msgpack::sbuffer> sw(buf);
    framework::jubatus_packer jp(sw);
    framework::packer packer(jp);
    unlearner_1.pack(packer);
  }

  {
    msgpack::unpacked unpacked;
    msgpack::unpack(&unpacked, buf.data(), buf.size());
    unlearner_2.unpack(unpacked.get());
  }

  for (size_t i = 0; i < touch_sequence.size(); ++i) {
    EXPECT_TRUE(unlearner_2.exists_in_memory(touch_sequence[i]));
  }

  unlearner_2.touch("id3");
  EXPECT_EQ("id1", callback_2.unlearned_id());
  EXPECT_FALSE(unlearner_2.exists_in_memory(callback_2.unlearned_id()));
}

}  // namespace unlearner
}  // namespace core
}  // namespace jubatus
