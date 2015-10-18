#include <vector>
#include <string>
#include <iostream>
#include "lsh_function.hpp"
#include "bit_vector_ranking.hpp"
#include "../common/type.hpp"
#include "../fv_converter/datum.hpp"
#include "../storage/bit_vector.hpp"
#include "../storage/column_type.hpp"
#include "../storage/abstract_column.hpp"

#include "jubatus/util/lang/cast.h"
#include "jubatus/util/math/random.h"
#include "jubatus/util/system/time_util.h"

namespace jubatus {
namespace core {
namespace nearest_neighbor {

std::vector<common::sfv_t>
generate_dataset(size_t num, size_t dencity) {
  // prepare dataset
  std::vector<common::sfv_t> data;
  data.reserve(num);
  util::math::random::mtrand rand;
  for (size_t i = 0; i < num; ++i) {
    common::sfv_t d;
    d.reserve(dencity);
    for (size_t j = 0; j < dencity; ++j) {
      d.push_back(
          std::make_pair(util::lang::lexical_cast<std::string>(j),
                         rand.next_double()));
    }
    data.push_back(d);
  }
  return data;
}

int bench() {
  size_t num = 100000;
  size_t dencity = 10;
  int ret = 0;
  int hash_num = 4096;
  std::vector<common::sfv_t> dataset = generate_dataset(num, dencity);
  storage::column_type conf(storage::column_type::bit_vector_type, hash_num);
  storage::bit_vector_column bc(conf);

  {  // insert
    util::system::time::clock_time start =
        util::system::time::get_clock_time();
    for (size_t i = 0; i < num; ++i) {
      bc.push_back(nearest_neighbor::cosine_lsh(dataset[i], hash_num));
    }
    util::system::time::clock_time finish =
        util::system::time::get_clock_time();
    double elapsed = static_cast<double>(finish - start);

    std::cout << "insertion: "
      <<  (elapsed / num)
      << " sec/hash with " << dencity << " dimentions" << std::endl
      << "total " << elapsed << " sec" << std::endl;
  }

  {  // search
    util::system::time::clock_time start =
        util::system::time::get_clock_time();
    bit_vector_ranker ranker_(16);
    storage::bit_vector bv(hash_num);
    bv.set_bit(303);
    std::vector<std::pair<uint64_t, float> > ret;
    const size_t num = 10;

    for (size_t i = 0; i < num; ++i) {
      ranker_.ranking_hamming_bit_vectors(bv, bc, ret, 16);
    }

    util::system::time::clock_time finish =
        util::system::time::get_clock_time();
    double elapsed = static_cast<double>(finish - start);

    std::cout << "search: "
      <<  (elapsed / num)
      << " sec/search with " << dencity << " dimentions" << std::endl
      << "total " << elapsed << " sec" << std::endl;
  }

  return ret;
}

}  // namespace nearest_neighbor
}  // namespace core
}  // namespace jubatus

int main() {
  return jubatus::core::nearest_neighbor::bench();
}
