#ifndef __RANDOM_H__
#define __RANDOM_H__

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/uniform_real_distribution.hpp>
#include <boost/random/random_device.hpp>

class Random {
public:
  static Random& instance() {
    static Random random;
    return random;
  }

  void initialize_with_random_seed() {
    typedef boost::random_device Dev;
    Dev generate;
    Dev::result_type random_number = generate();
    initialize_with_seed(random_number - Dev::min());
  }

  void initialize_with_seed(unsigned int seed) {
    _seed = seed;
    _generator.seed(_seed);
  }

  unsigned int get_seed() const { return _seed; }

  int generate_int(int from, int to) {
    boost::random::uniform_int_distribution<> dist(from, to);
    return dist(_generator);
  }

  bool generate_bool() {
    return generate_int(0, 1) == 1;
  }

  // TODO: The algorithm probably shouldn't use floats.
  float generate_float() {
    boost::random::uniform_real_distribution<> dist(0.f, 1.f);
    return dist(_generator);
  }

private:
  Random() {}

private:
  unsigned int _seed;
  boost::random::mt19937 _generator;
};

#endif // ifndef __RANDOM_H__
