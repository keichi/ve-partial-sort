// source /opt/nec/ve/nlc/2.3.0/bin/nlcvars.sh
// nc++-3.4.0 -Wall -O4 -finline-functions lsd_radix_sort.cpp -lasl_sequential
// && ./a.out
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

const int VLEN = 256;
#define MIN(a, b) ((a) < (b) ? (a) : (b))

void countsrt_vec(int n, uint32_t *v, uint32_t *r, int bits, int pos,
                  uint32_t *permin, uint32_t *permout)
{
    int i, j, k, m, digit, sum, val;
    int nb = 1 << bits;
    int shift = bits * pos;
    unsigned int mask = (nb - 1) << shift;
    int stride = n / VLEN;
    int bucket[nb][VLEN];

    if (stride * VLEN != n) stride++;
    stride = stride > 0 ? stride : 1;

    for (i = 0; i < nb * VLEN; i++) ((int *)bucket)[i] = 0;

    /* histogram keys */
    for (i = 0; i < stride; i++) {
#pragma _NEC ivdep
#pragma ivdep
        for (j = i, k = 0; j < MIN(i + VLEN * stride, n); j += stride, k++) {
            digit = (v[j] & mask) >> shift;
            bucket[digit][k] += 1;
        }
    }

    /* scan buckets */
    sum = 0;
    for (i = 0; i < nb * VLEN; i++) {
        val = ((int *)bucket)[i];
        ((int *)bucket)[i] = sum;
        sum += val;
    }

    /* rank and permute */
    for (i = 0; i < stride; i++) {
        for (j = i, k = 0; j < MIN(i + VLEN * stride, n); j += stride, k++) {
#pragma _NEC ivdep
#pragma ivdep
            digit = (v[j] & mask) >> shift;
            m = bucket[digit][k];
            r[m] = v[j];
            if (permin != NULL) permout[m] = permin[j];
            bucket[digit][k] = m + 1;
        }
    }
}

void radixsrt_vec(int n, uint32_t *v, uint32_t *r, int bits, uint32_t *perm,
                  uint32_t *ptmp)
{
    uint32_t *tmp;

    for (int pos = 0; pos < (sizeof(int) * 8) / bits; pos++) {
        countsrt_vec(n, v, r, bits, pos, perm, ptmp);

        tmp = v;
        v = r;
        r = tmp;
        if (perm != NULL) {
            tmp = perm;
            perm = ptmp;
            ptmp = tmp;
        }
    }
}

int main()
{
#ifdef __NEC__
    asl_library_initialize();
#endif

    for (auto n : NS) {
        for (auto k : KS) {
            Timer timer;

            std::vector<float> distances(n);
            std::vector<float> distances_work(n);
            std::vector<int> indices(n);
            std::vector<int> indices_work(n);

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
#endif

#ifdef _FTRACE
                ftrace_region_begin("lsd_radix_sort");
#endif
                timer.start();

                std::iota(indices.begin(), indices.end(), 0);

                radixsrt_vec(
                    n, reinterpret_cast<uint32_t *>(distances.data()),
                    reinterpret_cast<uint32_t *>(distances_work.data()), 8,
                    reinterpret_cast<uint32_t *>(indices.data()),
                    reinterpret_cast<uint32_t *>(indices_work.data()));

                timer.stop();
#ifdef _FTRACE
                ftrace_region_end("lsd_radix_sort");
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
