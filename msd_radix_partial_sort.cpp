// source /opt/nec/ve/nlc/2.3.0/bin/nlcvars.sh
// nc++-3.4.0 -Wall -O4 -finline-functions msd_radix_partial_sort.cpp
// -lasl_sequential && ./a.out
#include <algorithm>
#include <iostream>
#include <numeric>
#include <vector>

#include <asl.h>
#ifdef _FTRACE
#include <ftrace.h>
#endif

#include "timer.hpp"

const int NUM_TRIALS = 100;
std::vector<int> NS = {1000,  2000,   5000,   10000,  20000,
                       50000, 100000, 200000, 500000, 1000000};
std::vector<int> KS = {1, 2, 5, 10, 20, 50, 100};

const int VLEN = 256;

void radix_partial_sort(uint32_t *v, uint32_t *w, uint32_t *ix, uint32_t *ixw,
                        int lo, int hi, int k, int bits, int pos)
{
    int nb = 1 << bits;
    int shift = bits * pos;
    uint32_t mask = (nb - 1) << shift;
    int bucket[nb][VLEN];

    for (int i = 0; i < nb * VLEN; i++) ((int *)bucket)[i] = 0;

#pragma _NEC ivdep
    for (int i = lo; i < hi; i++) {
        uint32_t digit = (v[i] & mask) >> shift;
        bucket[digit][i % VLEN] += 1;
    }

    int sum = 0;
    for (int i = 0; i < nb * VLEN; i++) {
        int val = ((int *)bucket)[i];
        ((int *)bucket)[i] = sum;
        sum += val;
    }

#pragma _NEC ivdep
    for (int i = lo; i < hi; i++) {
        uint32_t digit = (v[i] & mask) >> shift;
        int m = bucket[digit][i % VLEN];
        w[m + lo] = v[i];
        ixw[m + lo] = ix[i];
        bucket[digit][i % VLEN] = m + 1;
    }

#pragma _NEC ivdep
    for (int i = lo; i < hi; i++) {
        v[i] = w[i];
        ixw[i] = ix[i];
    }

    for (int i = 0; i < nb; i++) {
        int start = i > 0 ? bucket[i - 1][VLEN - 1] : 0;
        int end = bucket[i][VLEN - 1];

        if (start + lo >= k) break;
        if (end - start <= 1) continue;
        if (end - start < 128) {
            for (int j = start + lo + 1; j < end + lo; j++) {
                int key = v[j];
                int k = j - 1;

                while (k >= 0 && v[k] > key) {
                    v[k + 1] = v[k];
                    ix[k + 1] = ix[k];
                    k = k - 1;
                }
                v[k + 1] = key;
            }

            continue;
        }

        radix_partial_sort(v, w, ix, ixw, start + lo, end + lo, k, bits,
                           pos - 1);
    }
}

int main()
{
    asl_library_initialize();

    for (auto n : NS) {
        for (auto k : KS) {
            Timer timer;

            std::vector<float> distances(n);
            std::vector<float> distances_work(n);
            std::vector<int> indices(n);
            std::vector<int> indices_work(n);

            asl_random_t rng;
            asl_random_create(&rng, ASL_RANDOMMETHOD_MT19937_64);
            asl_random_distribute_uniform(rng);

            for (int i = 0; i < NUM_TRIALS; i++) {
                asl_random_generate_s(rng, n, distances.data());

#ifdef _FTRACE
                ftrace_region_begin("msd_radix_partial_sort");
#endif
                timer.start();

                radix_partial_sort(
                    reinterpret_cast<uint32_t *>(distances.data()),
                    reinterpret_cast<uint32_t *>(distances_work.data()),
                    reinterpret_cast<uint32_t *>(indices.data()),
                    reinterpret_cast<uint32_t *>(indices_work.data()), 0, n, k,
                    4, 7);

                timer.stop();
#ifdef _FTRACE
                ftrace_region_end("msd_radix_partial_sort");
#endif
            }

            std::cout << n << " \t" << k << "\t" << timer.elapsed() / NUM_TRIALS
                      << std::endl;

            asl_random_destroy(rng);
        }
    }

    asl_library_finalize();
}
