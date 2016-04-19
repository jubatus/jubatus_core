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

#ifndef JUBATUS_CORE_STORAGE_BIT_VECTOR_HPP_
#define JUBATUS_CORE_STORAGE_BIT_VECTOR_HPP_

#include <stdint.h>
#include <algorithm>
#include <string>
#include <vector>

#include <msgpack.hpp>
#include "jubatus/util/lang/cast.h"
#include "../common/assert.hpp"
#include "../common/exception.hpp"
#include "../common/big_endian.hpp"
#include "storage_exception.hpp"

namespace jubatus {
namespace core {
namespace storage {
namespace detail {

template <typename T, size_t N> struct bitcount_impl;
template <typename T>
struct bitcount_impl<T, 1> {
  static int call(T bits) {
    bits = (bits & 0x55) + (bits >> 1 & 0x55);
    bits = (bits & 0x33) + (bits >> 2 & 0x33);
    return (bits & 0x0f) + (bits >> 4 & 0x0f);
  }
};
template <typename T>
struct bitcount_impl<T, 2> {
  static int call(T bits) {
    bits = (bits & 0x5555U) + (bits >> 1 & 0x5555U);
    bits = (bits & 0x3333U) + (bits >> 2 & 0x3333U);
    bits = (bits & 0x0f0fU) + (bits >> 4 & 0x0f0fU);
    return (bits & 0x00ffU) + (bits >> 8 & 0x00ffU);
  }
};
template <typename T>
struct bitcount_impl<T, 4> {
  static int call(T bits) {
    bits = (bits & 0x55555555LU) + (bits >> 1 & 0x55555555LU);
    bits = (bits & 0x33333333LU) + (bits >> 2 & 0x33333333LU);
    bits = (bits & 0x0f0f0f0fLU) + (bits >> 4 & 0x0f0f0f0fLU);
    bits = (bits & 0x00ff00ffLU) + (bits >> 8 & 0x00ff00ffLU);
    return (bits & 0x0000ffffLU) + (bits >>16 & 0x0000ffffLU);
  }
};
template <typename T>
struct bitcount_impl<T, 8> {
  static int call(T bits) {
    bits = (bits & 0x5555555555555555LLU) + (bits >> 1 & 0x5555555555555555LLU);
    bits = (bits & 0x3333333333333333LLU) + (bits >> 2 & 0x3333333333333333LLU);
    bits = (bits & 0x0f0f0f0f0f0f0f0fLLU) + (bits >> 4 & 0x0f0f0f0f0f0f0f0fLLU);
    bits = (bits & 0x00ff00ff00ff00ffLLU) + (bits >> 8 & 0x00ff00ff00ff00ffLLU);
    bits = (bits & 0x0000ffff0000ffffLLU) + (bits >>16 & 0x0000ffff0000ffffLLU);
    return (bits & 0x00000000ffffffffLLU) + (bits >>32 & 0x00000000ffffffffLLU);
  }
};

template <class T>
inline int bitcount_dispatcher(T bits) {
  return bitcount_impl<T, sizeof(T)>::call(bits);
}

inline int bitcount(unsigned bits) {
  return bitcount_dispatcher(bits);
}

inline int bitcount(unsigned long bits) {  // NOLINT
  return bitcount_dispatcher(bits);
}

inline int bitcount(unsigned long long bits) {  // NOLINT
  return bitcount_dispatcher(bits);
}

template <class T>
inline int bitcount(T);  // = delete;

size_t calc_hamming_distance_internal(
    const uint64_t *x, const uint64_t *y, size_t blocks);
size_t bit_count_internal(const uint64_t *x, size_t blocks);

}  // namespace detail

class bit_vector_unmatch_exception
    : public common::exception::jubaexception<bit_vector_unmatch_exception> {
 public:
  explicit bit_vector_unmatch_exception(const std::string &msg)
      : msg_(msg) {}
  ~bit_vector_unmatch_exception() throw() {}
  const char* what() const throw() {
    return msg_.c_str();
  }

 private:
  std::string msg_;
};

template <typename bit_base>
struct bit_vector_base {
  static const size_t BASE_BITS = sizeof(bit_base) * 8;
  static const size_t BLOCKSIZE = sizeof(bit_base);

  bit_vector_base()
    : bits_(NULL), bit_num_(0), own_(false) {
  }

  bit_vector_base(const bit_base* bits, size_t bit_num)
      : bits_(NULL),
        bit_num_(bit_num),
        own_(false) {
    alloc_memory();
    memcpy(bits_, bits, used_bytes());
  }

  explicit bit_vector_base(int bit_num)
      : bits_(NULL),
        bit_num_(bit_num),
        own_(false) {
  }

  bit_vector_base(const bit_vector_base& orig)
      : bits_(NULL),
        bit_num_(orig.bit_num_),
        own_(false) {
    if (orig.bits_ == NULL) {
      return;
    }
    alloc_memory();
    memcpy(bits_, orig.bits_, used_bytes());
  }

  ~bit_vector_base() {
    if (bits_ && own_) {
      release();
      return;
    }
  }

  void resize_and_clear(size_t bit_num) {
    release();
    own_ = false;
    bit_num_ = bit_num;
  }

  bool operator==(const bit_vector_base& rhs) const {
    if (bit_num_ != rhs.bit_num_) {
      return false;
    }
    if (is_empty() && rhs.is_empty()) {
      return true;
    }
    if (is_empty() && !rhs.is_empty()) {
      return false;
    }
    if (!is_empty() && rhs.is_empty()) {
      return false;
    }
    return memcmp(bits_, rhs.bits_, used_bytes()) == 0;
  }
  bool operator!=(const bit_vector_base& rhs) const {
    return !this->operator==(rhs);
  }

  // deep copy (In case not own memory, it alloc memory)
  bit_vector_base& operator=(const bit_vector_base& orig) {
    if (&orig != this) {
      bit_num_ = orig.bit_num_;
      if (bits_ == NULL) {
        alloc_memory();
      }
      if (orig.bits_ == NULL) {
        memset(bits_, 0, used_bytes());
      } else {
        memcpy(bits_, orig.bits_, used_bytes());
      }
    }
    return *this;
  }

  void swap(bit_vector_base& x) {
    using std::swap;
    swap(bit_num_, x.bit_num_);
    swap(bits_, x.bits_);
    swap(own_, x.own_);
  }
  friend void swap(bit_vector_base& l, bit_vector_base& r) {
    l.swap(r);
  }

  void clear_bit(size_t pos) {
    if (bits_ == NULL) {
      return;
    }
    if (bit_num_ <= pos) {
      throw bit_vector_unmatch_exception(
          "clear_bit(): invalid position " +
          jubatus::util::lang::lexical_cast<std::string>(pos) +
          " for length: " +
          jubatus::util::lang::lexical_cast<std::string>(bit_num_));
    }
    bits_[pos / BASE_BITS] &= ~(1LLU << (pos % BASE_BITS));
  }
  void set_bit(size_t pos) {
    if (bits_ == NULL) {
      alloc_memory();
    }
    if (bit_num_ <= pos) {
      throw bit_vector_unmatch_exception(
          "set_bit(): invalid position " +
          jubatus::util::lang::lexical_cast<std::string>(pos) +
          " for length: " +
          jubatus::util::lang::lexical_cast<std::string>(bit_num_));
    }
    bits_[pos / BASE_BITS] |= (1LLU << (pos % BASE_BITS));
  }
  void reverse_bit(size_t pos) {
    if (bits_ == NULL) {
      alloc_memory();
    }
    if (bit_num_ <= pos) {
      throw bit_vector_unmatch_exception(
          "reverse_bit(): invalid position " +
          jubatus::util::lang::lexical_cast<std::string>(pos) +
          " for length: " +
          jubatus::util::lang::lexical_cast<std::string>(bit_num_));
    }
    bits_[pos / BASE_BITS] ^= (1LLU << (pos % BASE_BITS));
  }

  bool get_bit(size_t pos) const {
    if (bits_ == NULL) {
      return false;
    }
    if (bit_num_ <= pos) {
      throw bit_vector_unmatch_exception(
          "get_bit(): invalid position " +
          jubatus::util::lang::lexical_cast<std::string>(pos) +
          " for length: " +
          jubatus::util::lang::lexical_cast<std::string>(bit_num_));
    }
    return bits_[pos / BASE_BITS] & (1LLU << (pos % BASE_BITS));
  }

  /**
   * Returns true if all the bits in this vector are 0.
   * If this vector is not initialized yet, returns true.
   */
  bool is_empty() const {
    if (bits_ == NULL) {
      return true;
    }
    for (size_t i = 0, blocks = used_bytes() / sizeof(bit_base);
         i < blocks; ++i) {
      if (bits_[i] != 0) {
        return false;
      }
    }
    return true;
  }

  void clear() {
    for (size_t i = 0; i < bit_num_; ++i) {
      clear_bit(i);
    }
  }
  uint64_t calc_hamming_similarity(const bit_vector_base& bv) const {
    return bit_num() - calc_hamming_distance(bv);
  }

  /**
   * Returns hamming distance between this vector and given vector.
   * Uninitialized vectors are considered as zero-initialized.
   */
  uint64_t calc_hamming_distance(const bit_vector_base& bv) const {
    if (bit_num() != bv.bit_num()) {
      throw bit_vector_unmatch_exception(
          "calc_hamming_similarity(): bit_vector length unmatch! " +
          jubatus::util::lang::lexical_cast<std::string>(bit_num()) + " with " +
          jubatus::util::lang::lexical_cast<std::string>(bv.bit_num()));
    }
    if (bits_ == NULL) {
      return bv.bit_count();
    } else if (bv.bits_ == NULL) {
      return bit_count();
    }
    return calc_hamming_distance_unsafe(bv.bits_);
  }
  uint64_t calc_hamming_distance_unsafe(const bit_base *bv) const {
    if (bits_ == NULL)
      return bit_count_unsafe(bv, used_bytes());
    return detail::calc_hamming_distance_internal(
        bits_, bv, used_bytes() / sizeof(bit_base));
  }
  size_t bit_count() const {
    return bit_count_unsafe(bits_, used_bytes());
  }

  bit_base* raw_data_unsafe() {
    return bits_;
  }
  const bit_base* raw_data_unsafe() const {
    return bits_;
  }
  size_t bit_num() const {
    return bit_num_;
  }
  size_t used_bytes() const {
    return memory_size(bit_num_);
  }
  static size_t memory_size(size_t bit_width) {
    return ((((bit_width + 7) / 8) + BLOCKSIZE - 1) / BLOCKSIZE) * BLOCKSIZE;
  }

  void debug_print(std::ostream& os) const {
    if (bits_ == NULL) {
      os << "(unallocated)";
      return;
    }
    for (uint64_t i = 0; i < used_bytes() * 8; ++i) {
      if ((bits_[i / (sizeof(bit_base) * 8)] >> (i % (sizeof(bit_base) * 8))) &
          1LLU) {
        os << "1";
      } else {
        os << "0";
      }
    }
  }
  friend std::ostream& operator<<(std::ostream& os, const bit_vector_base& bv) {
    bv.debug_print(os);
    return os;
  }
  void status(std::ostream& os) const {
    os << "status():";
    if (bits_ == NULL) {
      os << "(unallocated)" << " owns_" << own_;
      return;
    }
    os << (own_ ? "[own]" : "[other]") << std::endl;
  }

  template <typename packable>
  void pack(packable& buffer) const {
    msgpack::pack(buffer, *this);
  }
  template<typename Buffer>
  void msgpack_pack(msgpack::packer<Buffer>& packer) const {
    packer.pack_array(2);
    packer.pack(static_cast<uint64_t>(bit_num_));
    packer.pack_raw(used_bytes());
    if (bits_) {
      const size_t n = used_bytes() / BLOCKSIZE;
      for (size_t i = 0; i < n; ++i) {
        char buf[BLOCKSIZE];
        common::write_big_endian(bits_[i], buf);
        packer.pack_raw_body(buf, BLOCKSIZE);
      }
    } else {
      const char c = 0;
      for (size_t i = 0; i < used_bytes(); ++i) {
        packer.pack_raw_body(&c, 1);
      }
    }
  }
  void msgpack_unpack(msgpack::object o) {
    if (o.type != msgpack::type::ARRAY || o.via.array.size != 2) {
      throw msgpack::type_error();  // like MSGPACK_DEFINE
    }
    const msgpack::object* objs = o.via.array.ptr;
    const uint64_t bit_num = objs[0].as<uint64_t>();
    const msgpack::type::raw_ref data = objs[1].as<msgpack::type::raw_ref>();
    if (data.size != memory_size(bit_num)) {
      throw bit_vector_unmatch_exception(
          "msgpack_unpack(): invalid length of packed data: "
          "expected: " +
          jubatus::util::lang::lexical_cast<std::string>(bit_num_) +
          ", got: " +
          jubatus::util::lang::lexical_cast<std::string>(data.size));
    }
    std::vector<bit_base> buf;
    for (size_t i = 0; i < data.size; i += BLOCKSIZE) {
      buf.push_back(common::read_big_endian<bit_base>(&data.ptr[i]));
    }
    bit_vector_base(&buf[0], bit_num).swap(*this);
    JUBATUS_ASSERT(own_ || bits_ == NULL);
  }

  void alloc_memory() {
    JUBATUS_ASSERT(!own_);
    bits_ = new bit_base[used_bytes() / sizeof(bit_base)];
    own_ = true;
    memset(bits_, 0, used_bytes());
  }

 private:
  bool release() {
    if (bits_ && own_) {
      delete[] bits_;
      own_ = false;
      bits_ = NULL;
      return true;
    }
    return false;
  }

  // deep copy
  void duplicate(const bit_vector_base& orig) {
    if (bit_num_ != orig.bit_num_) {
      throw bit_vector_unmatch_exception(
          "failed copy bit vector from " +
          jubatus::util::lang::lexical_cast<std::string>(orig.bit_num_) +
          " to " +
          jubatus::util::lang::lexical_cast<std::string>(bit_num_));
    }
    if (!own_) {
      alloc_memory();
    }
    memcpy(bits_, orig.bits_, used_bytes());
  }

  static size_t bit_count_unsafe(const bit_base *bits, size_t bytes) {
    if (bits == NULL) {
      return 0;
    }
    return detail::bit_count_internal(bits, bytes / sizeof(bit_base));
  }

  bit_base* bits_;
  size_t bit_num_;
  bool own_;
};
typedef bit_vector_base<uint64_t> bit_vector;

}  // namespace storage
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_STORAGE_BIT_VECTOR_HPP_
