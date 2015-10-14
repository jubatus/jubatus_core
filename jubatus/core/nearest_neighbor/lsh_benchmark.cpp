#include <vector>
#include <string>
#include <iostream>
#include "lsh_function.hpp"
#include "bit_vector_ranking.hpp"
#include "nearest_neighbor_factory.hpp"
#include "nearest_neighbor_base.hpp"
#include "../common/type.hpp"
#include "../common/jsonconfig.hpp"
#include "../fv_converter/datum.hpp"
#include "../storage/bit_vector.hpp"
#include "../storage/column_type.hpp"
#include "../storage/column_table.hpp"
#include "../storage/abstract_column.hpp"

#include "jubatus/util/lang/cast.h"
#include "jubatus/util/lang/shared_ptr.h"
#include "jubatus/util/math/random.h"
#include "jubatus/util/system/time_util.h"

using jubatus::util::system::time::clock_time;
using jubatus::util::system::time::get_clock_time;
using jubatus::util::lang::lexical_cast;
using jubatus::util::lang::shared_ptr;
using jubatus::core::nearest_neighbor::cosine_lsh;
using jubatus::core::nearest_neighbor::create_nearest_neighbor;
using jubatus::core::nearest_neighbor::nearest_neighbor_base;
using jubatus::core::common::sfv_t;
using jubatus::core::common::jsonconfig::config;
using jubatus::core::storage::column_table;
using jubatus::core::storage::column_type;
using jubatus::core::storage::bit_vector;
using jubatus::core::storage::bit_vector_column;
using jubatus::core::nearest_neighbor::bit_vector_ranker;
using jubatus::util::text::json::json;
using jubatus::util::text::json::json_null;
using jubatus::util::text::json::json_integer;
using jubatus::util::text::json::json_object;
using jubatus::util::text::json::to_json;

namespace {
sfv_t generate_sfv(size_t dencity, jubatus::util::math::random::mtrand& rand) {
  sfv_t d;
  d.reserve(dencity);
  for (size_t i = 0; i < dencity; ++i) {
    d.push_back(
      std::make_pair(lexical_cast<std::string>(i),
                     rand.next_double()));
  }
  return d;
}

std::vector<sfv_t> generate_dataset(size_t num, size_t dencity) {
  // prepare dataset
  std::vector<sfv_t> data;
  data.reserve(num);
  jubatus::util::math::random::mtrand rand;
  for (size_t i = 0; i < num; ++i) {
    data.push_back(generate_sfv(dencity, rand));
  }
  return data;
}

bit_vector random_bit_vector(size_t bit_num) {
  jubatus::util::math::random::mtrand rand;
  bit_vector bv(bit_num);
  for (size_t i = 0; i < 10; ++i) {
    bv.set_bit(rand(bit_num));
  }
  return bv;
}

typedef std::pair<double, double> benchtime_t;
class micro_timer {
 public:
  micro_timer() :start_(get_clock_time()) {}
  double elapsed() const {
    clock_time now = get_clock_time();
    return static_cast<double>(now - start_);
  }
 private:
  clock_time start_;
};

}  // anonymous namespace

benchtime_t nn_bench(
    const std::string& type,
    const std::vector<sfv_t>& dataset,
    size_t threads,
    size_t num,
    size_t dencity,
    int ret_num,
    int hash_num) {
  json js(new json_object);
  js["hash_num"] = new json_integer(hash_num);
  config conf(js);
  shared_ptr<column_table> table(new column_table);
  const std::string id("id");

  shared_ptr<nearest_neighbor_base> nn =
    create_nearest_neighbor(type,
                            conf,
                            table,
                            "id");

  double insert_time;
  {  // insert
    micro_timer timer;
    for (size_t i = 0; i < num; ++i) {
      nn->set_row(lexical_cast<std::string>(i), dataset[i]);
    }
    insert_time = timer.elapsed();
  }

  double search_time;
  {  // search
    jubatus::util::math::random::mtrand rand;
    sfv_t query = generate_sfv(dencity, rand);
    std::vector<std::pair<std::string, float> > ret;
    size_t tries = 1000;

    micro_timer timer;
    for (size_t i = 0; i < tries; ++i) {      // 100 times
      nn->neighbor_row(query, ret, ret_num);
    }
    search_time = timer.elapsed() / tries;
  }

  return std::make_pair(insert_time, search_time);
}

void dump_time(
    std::string& name,
    const benchtime_t& time,
    size_t num,
    size_t dencity,
    size_t hash_num) {
  std::cout << "=== target: " << name << " ====" << std::endl
      << "insertion: "
      <<  (time.first / num)
      << " sec/hash with " << dencity << " dimentions for "
      << hash_num << " bit hash "
      << "calclurated with " << num << " entry" << std::endl
      << "total " << time.first << " sec" << std::endl;
  std::cout << "search: "
      <<  (time.second / num)
      << " sec/search with " << dencity << " dimentions for "
      << hash_num << " bit hash "
      << "calclurated with " << num << " entry" << std::endl
      << "total " << time.second << " sec" << std::endl << std::flush;
}

int main() {
  size_t num = 100000;
  size_t threads = 16;
  size_t dencity = 100;
  int ret_num = 16;
  int hash_num = 4096;

  std::cout << "perparing dataset :"
            << num << " entries "
            << dencity << " dimentions" << std::endl;
  std::vector<sfv_t> dataset =
      generate_dataset(num, dencity);

  std::vector<std::string> types;
  //types.push_back("minhash");
  types.push_back("lsh");
  //types.push_back("euclid_lsh");

  for (size_t i = 0; i < types.size(); ++i) {
    std::cout << "benchmarking :" << types[i] << std::endl;
    std::pair<double, double> time = nn_bench(
        types[i],
        dataset,
        threads,
        num,
        dencity,
        ret_num,
        hash_num);
    dump_time(types[i], time, num, dencity, hash_num);
  }
  return 0;
}
