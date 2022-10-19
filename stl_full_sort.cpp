// source /opt/nec/ve/nlc/2.3.0/bin/nlcvars.sh
// nc++-3.4.0 -Wall -O4 -finline-functions stl_full_sort.cpp -lasl_sequential &&
// ./a.out
#include <algorithm>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

#ifdef __NEC__
#include <asl.h>
#ifdef _FTRACE
#include <ftrace.h>
#endif
#endif

#include "timer.hpp"

const int NUM_TRIALS = 100;
std::vector<int> NS = {1000,  2000,   5000,   10000,  20000,
                       50000, 100000, 200000, 500000, 1000000};
std::vector<int> KS = {1, 2, 5, 10, 20, 50, 100};

int main()
{
#ifdef __NEC__
    asl_library_initialize();
#endif

    for (auto n : NS) {
        for (auto k : KS) {
            Timer timer;

            std::vector<float> distances(n);
            std::vector<int> indices(n);

#ifdef __NEC__
            asl_random_t rng;
            asl_random_create(&rng, ASL_RANDOMMETHOD_MT19937_64);
            asl_random_distribute_uniform(rng);
#else
            std::random_device seed_gen;
            std::mt19937 engine(seed_gen());
            std::uniform_real_distribution<float> dist(0.0f, 1.0f);
#endif

            for (int i = 0; i < NUM_TRIALS; i++) {
#ifdef __NEC__
                asl_random_generate_s(rng, n, distances.data());
#else
                std::generate(distances.begin(), distances.end(),
                              [&] { return dist(engine); });
#endif

#ifdef _FTRACE
                ftrace_region_begin("stl_full_sort");
#endif
                timer.start();

                std::iota(indices.begin(), indices.end(), 0);
                std::sort(indices.begin(), indices.end(),
                          [&](int a, int b) -> int {
                              return distances[a] < distances[b];
                          });
                timer.stop();
#ifdef _FTRACE
                ftrace_region_end("stl_full_sort");
#endif
            }

            std::cout << n << " \t" << k << "\t" << timer.elapsed() / NUM_TRIALS
                      << std::endl;

#ifdef __NEC__
            asl_random_destroy(rng);
#endif
        }
    }

#ifdef __NEC__
    asl_library_finalize();
#endif
}
