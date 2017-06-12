// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2012,2013 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#include "gtest/gtest.h"
#include "bit_vector.hpp"

using jubatus::core::storage::bit_vector;
using jubatus::core::storage::bit_vector_base;

TEST(bit_vector, length) {
  bit_vector bv(80);
  EXPECT_EQ(bv.bit_num(), 80U);
}

TEST(bit_vector, bounds_checking) {
  bit_vector bv(80);

  // reading/writing out-of-bound bits should throw exception
  EXPECT_THROW(bv.set_bit(bv.bit_num()),
               jubatus::core::storage::bit_vector_unmatch_exception);
  EXPECT_THROW(bv.get_bit(bv.bit_num()),
               jubatus::core::storage::bit_vector_unmatch_exception);
  EXPECT_THROW(bv.reverse_bit(bv.bit_num()),
               jubatus::core::storage::bit_vector_unmatch_exception);
  EXPECT_THROW(bv.clear_bit(bv.bit_num()),
               jubatus::core::storage::bit_vector_unmatch_exception);

  // writing bits for uninitialized vector should throw exception
  // get_bit/clear_bit should do nothing for uninitialized vector
  EXPECT_THROW(bit_vector().set_bit(0),
               jubatus::core::storage::bit_vector_unmatch_exception);
  EXPECT_NO_THROW(bit_vector().get_bit(0));
  EXPECT_THROW(bit_vector().reverse_bit(0),
               jubatus::core::storage::bit_vector_unmatch_exception);
  EXPECT_NO_THROW(bit_vector().clear_bit(0));
}

TEST(bit_vector, set) {
  bit_vector bv(80);
  for (size_t i = 0; i < bv.bit_num(); ++i) {
    bv.set_bit(i);
  }
}

TEST(bit_vector, is_empty) {
  bit_vector bv(80);
  ASSERT_TRUE(bv.is_empty());
  bv.set_bit(4);
  ASSERT_FALSE(bv.is_empty());
  bv.clear_bit(4);
  ASSERT_TRUE(bv.is_empty());

  // uninitialized bit_vectors must be empty
  ASSERT_TRUE(bit_vector().is_empty());
}

TEST(bit_vector, set_get) {
  for (size_t k = 10; k < 67; ++k) {
    bit_vector bv(k);
    for (size_t j = 0; j < k; ++j) {
      bv.set_bit(j);
      ASSERT_TRUE(bv.get_bit(j));
      for (size_t i = 0; i < bv.bit_num(); ++i) {
        if (i  <= j) {
          ASSERT_TRUE(bv.get_bit(i));
        } else {
          ASSERT_FALSE(bv.get_bit(i));
        }
      }
    }
  }
}

TEST(bit_vector, set_get_clear) {
  for (size_t k = 10; k < 67; ++k) {
    bit_vector bv(k);
    for (size_t j = 0; j < k; ++j) {
      bv.set_bit(j);
      ASSERT_TRUE(bv.get_bit(j));
      for (size_t i = 0; i < bv.bit_num(); ++i) {
        if (i == j) {
          ASSERT_TRUE(bv.get_bit(i));
        } else {
          ASSERT_FALSE(bv.get_bit(i));
        }
      }
      bv.clear_bit(j);
    }
  }
}

TEST(bit_vector, big_vector) {
  const size_t size = 4100;
  bit_vector bv(size);
  for (size_t i = 0; i < size; ++i) {
    bv.set_bit(i);
    for (size_t j = 0; j <= i; ++j) {
      ASSERT_TRUE(bv.get_bit(j));
    }
    for (size_t j = i+1; j < size; ++j) {
      ASSERT_FALSE(bv.get_bit(j));
    }
  }
  for (size_t i = 0; i < size; ++i) {
    bv.clear_bit(i);
    for (size_t j = 0; j <= i; ++j) {
      ASSERT_FALSE(bv.get_bit(j));
    }
    for (size_t j = i+1; j < size; ++j) {
      ASSERT_TRUE(bv.get_bit(j));
    }
  }
}

TEST(bit_vector, empty_copy) {
  for (size_t k = 10; k < 200; ++k) {
    bit_vector bv1(k);
    for (size_t j = 0; j < k; ++j) {
      ASSERT_FALSE(bv1.get_bit(j));
    }

    bit_vector bv2(k);
    for (size_t j = 0; j < k; ++j) {
      ASSERT_FALSE(bv2.get_bit(j));
    }
    bv2 = bv1;
    for (size_t j = 0; j < k; ++j) {
      ASSERT_FALSE(bv2.get_bit(j));
    }
  }
}

TEST(bit_vector, copy) {
  for (size_t k = 10; k < 67; ++k) {
    bit_vector bv(k);
    for (size_t j = 0; j < k; ++j) {
      bv.set_bit(j);
      bit_vector b(k);
      b = bv;
      ASSERT_TRUE(b == bv);
      ASSERT_TRUE(b.get_bit(j));
      for (size_t i = 0; i < b.bit_num(); ++i) {
        if (bv.get_bit(i)) {
          ASSERT_TRUE(b.get_bit(i));
        } else {
          ASSERT_FALSE(b.get_bit(i));
        }
      }
      ASSERT_TRUE(b == bv);
      bv.clear_bit(j);
    }
  }
}

TEST(bit_vector, copy_constructor) {
  for (size_t k = 10; k < 67; ++k) {
    bit_vector bv(k);
    for (size_t j = 0; j < k; ++j) {
      bv.set_bit(j);
      bit_vector b(bv);
      ASSERT_TRUE(b == bv);
      ASSERT_TRUE(b.get_bit(j));
      for (size_t i = 0; i < b.bit_num(); ++i) {
        if (bv.get_bit(i)) {
          ASSERT_TRUE(b.get_bit(i));
        } else {
          ASSERT_FALSE(b.get_bit(i));
        }
      }
      ASSERT_TRUE(b == bv);
      bv.clear_bit(j);
    }
  }
}

TEST(bit_vector, copy_constructor_many_bit) {
  for (size_t k = 10; k < 67; ++k) {
    bit_vector bv(k);
    for (size_t j = 0; j < k; ++j) {
      bv.set_bit(j);
      bit_vector b(bv);
      ASSERT_TRUE(b == bv);
      ASSERT_TRUE(b.get_bit(j));
      for (size_t i = 0; i < b.bit_num(); ++i) {
        if (bv.get_bit(i)) {
          ASSERT_TRUE(b.get_bit(i));
        } else {
          ASSERT_FALSE(b.get_bit(i));
        }
      }
      ASSERT_TRUE(b == bv);
    }
  }
}
TEST(bit_count, simply_count) {
  for (size_t i = 1; i < 200; ++i) {
    bit_vector bv(i);
    for (size_t j = 0; j < i; ++j) {
      ASSERT_EQ(j, bv.bit_count());
      bv.set_bit(j);
      ASSERT_EQ(j+1, bv.bit_count());
    }
  }
}
TEST(hamming_similarity, calc_with_empty) {
  for (size_t k = 10; k < 67; ++k) {
    bit_vector bv1(k);
    bit_vector bv2(k);
    for (size_t j = 0; j < k; ++j) {
      ASSERT_EQ(k-j, bv1.calc_hamming_similarity(bv2));
      bv1.set_bit(j);
    }
  }
}

TEST(hamming_similarity, calc_with_one) {
  for (size_t k = 10; k < 67; ++k) {
    bit_vector bv1(k);
    bv1.set_bit(0);
    bit_vector bv2(k);
    for (size_t j = 1; j < k; ++j) {
      // std::cout << bv1 << std::endl << bv2 << std::endl;
      ASSERT_EQ(k-1, bv1.calc_hamming_similarity(bv2));
      bv1.set_bit(j);
      bv2.set_bit(j);
    }
  }
}

TEST(hamming_similarity, calc_with_two) {
  for (size_t k = 10; k < 67; ++k) {
    bit_vector bv1(k);
    bv1.set_bit(0);
    bit_vector bv2(k);
    bit_vector bv3(k);
    for (size_t j = 1; j < k; ++j) {
      ASSERT_EQ(k-j, bv1.calc_hamming_similarity(bv2));
      ASSERT_EQ(k-j, bv1.calc_hamming_similarity(bv3));
      ASSERT_EQ(k, bv2.calc_hamming_similarity(bv3));
      bv2.set_bit(j);
      bv3.set_bit(j);
    }
  }
}
TEST(hamming_similarity, calc_with_skew) {
  for (size_t k = 10; k < 67; ++k) {
    bit_vector bv1(k);
    bit_vector bv2(k);
    for (size_t j = 0; j < k; ++j) {
      bv1.set_bit(j);
      ASSERT_EQ(k-1, bv1.calc_hamming_similarity(bv2));
      bv2.set_bit(j);
    }
  }
}
TEST(memory_size, uint64_under_128) {
  for (size_t k = 1; k <= 64; ++k) {
    bit_vector_base<uint64_t> bv(k);
    ASSERT_EQ(bv.used_bytes(), 8U);
  }
  for (size_t k = 65; k <= 128; ++k) {
    bit_vector_base<uint64_t> bv(k);
    ASSERT_EQ(bv.used_bytes(), 16U);
  }
}
TEST(memory_size, uint8_under_128) {
  for (size_t j = 0; j < 128/8; ++j) {
    for (size_t k = j*8+1; k <= (j+1)*8; ++k) {
      bit_vector_base<uint8_t> bv(k);
      ASSERT_EQ(bv.used_bytes(), j+1);
    }
  }
}

TEST(construct, vector_resize) {
  std::vector<bit_vector> bv(10, bit_vector(10));
  bv.resize(100, bit_vector(19));
}

TEST(msgpack_pack, empty) {
  const bit_vector bv(10);
  EXPECT_EQ(NULL, bv.raw_data_unsafe());

  msgpack::sbuffer buf;
  msgpack::pack(buf, bv);

  msgpack::zone z;
  msgpack::object obj = msgpack::unpack(z, buf.data(), buf.size());

  ASSERT_EQ(msgpack::type::ARRAY, obj.type);
  ASSERT_EQ(2u, obj.via.array.size);

  const size_t bits = obj.via.array.ptr[0].as<uint64_t>();
  const std::string data = obj.via.array.ptr[1].as<std::string>();

  EXPECT_EQ(bv.bit_num(), bits);

  EXPECT_EQ(bv.used_bytes(), data.size());
  for (size_t i = 0; i < data.size(); ++i) {
    EXPECT_EQ(0, data[i]);
  }
}
TEST(msgpack_pack, simple) {
  bit_vector bv(10);
  bv.set_bit(5);
  EXPECT_TRUE(bv.raw_data_unsafe() != NULL);

  msgpack::sbuffer buf;
  msgpack::pack(buf, bv);

  msgpack::zone z;
  msgpack::object obj = msgpack::unpack(z, buf.data(), buf.size());

  ASSERT_EQ(msgpack::type::ARRAY, obj.type);
  ASSERT_EQ(2u, obj.via.array.size);

  const size_t bits = obj.via.array.ptr[0].as<uint64_t>();
  const std::string data = obj.via.array.ptr[1].as<std::string>();

  EXPECT_EQ(bv.bit_num(), bits);

  EXPECT_EQ(bv.used_bytes(), data.size());
  const uint64_t* orig_data = bv.raw_data_unsafe();
  for (size_t i = 0; i < bv.used_bytes() / bv.BLOCKSIZE; ++i) {
    uint64_t x = jubatus::core::common::read_big_endian<uint64_t>(&data[i]);
    EXPECT_EQ(orig_data[i], x);
  }
}
TEST(msgpack_unpack, packed) {
  bit_vector orig(10);
  orig.set_bit(5);
  EXPECT_TRUE(orig.raw_data_unsafe() != NULL);

  msgpack::sbuffer buf;
  msgpack::pack(buf, orig);

  msgpack::zone z;
  msgpack::object o = msgpack::unpack(z, buf.data(), buf.size());

  bit_vector copied(10);
  o.convert(copied);

  EXPECT_EQ(orig, copied);
}
