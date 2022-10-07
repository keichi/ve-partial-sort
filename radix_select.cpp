#include <algorithm>
#include <cassert>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

#define VLEN 256

void counting_sort(int n, uint32_t *v, uint32_t *r, int bits, int pos, int k,
                   int *new_n, uint32_t **new_v, int *new_k)
{
    int nb = 1 << bits;
    int shift = bits * pos;
    uint32_t mask = (nb - 1) << shift;
    int stride = n / VLEN;
    int bucket[nb][VLEN];

    if (stride * VLEN != n) stride++;
    stride = stride > 0 ? stride : 1;

    for (int i = 0; i < nb * VLEN; i++) ((int *)bucket)[i] = 0;

    /* histogram keys */
    for (int i = 0; i < stride; i++) {
#pragma _NEC ivdep
        for (int j = i, k = 0; j < std::min(i + VLEN * stride, n);
             j += stride, k++) {
            uint32_t digit = (v[j] & mask) >> shift;
            bucket[digit][k] += 1;
        }
    }

    /* scan buckets */
    int sum = 0;
    for (int i = 0; i < nb * VLEN; i++) {
        int val = ((int *)bucket)[i];
        ((int *)bucket)[i] = sum;
        sum += val;
    }

    *new_k = k - bucket[nb - 1][0];
    *new_v = r + bucket[nb - 1][0];
    *new_n = n - bucket[nb - 1][0];

    for (int i = 0; i < nb; i++) {
        if (k <= bucket[i][0]) {
            *new_k = k - bucket[i - 1][0];
            *new_v = r + bucket[i - 1][0];
            *new_n = bucket[i][0] - bucket[i - 1][0];
            break;
        }
    }

    /* rank and permute */
    for (int i = 0; i < stride; i++) {
        for (int j = i, k = 0; j < std::min(i + VLEN * stride, n);
             j += stride, k++) {
#pragma _NEC ivdep
            uint32_t digit = (v[j] & mask) >> shift;
            int m = bucket[digit][k];
            r[m] = v[j];
            bucket[digit][k] = m + 1;
        }
    }
}

uint32_t radix_select(int n, uint32_t *v, uint32_t *r, int bits, int k)
{
    uint32_t *tmp;

    int new_n, new_k;
    uint32_t *new_v;

    for (int pos = (sizeof(uint32_t) * 8) / bits - 1; pos >= 0; pos--) {
        counting_sort(n, v, r, bits, pos, k, &new_n, &new_v, &new_k);

        tmp = v;
        v = r;
        r = tmp;

        n = new_n;
        v = new_v;
        k = new_k;

        if (n == 1) break;
    }

    return v[0];
}

int main()
{
    std::mt19937 engine(42);
    std::uniform_int_distribution<int> dist(1, 10000);

    for (int iter = 0; iter < 100; iter++) {
        int n = dist(engine);

        std::uniform_int_distribution<int> dist2(1, n);

        int k = dist2(engine);

        std::vector<uint32_t> input(n);
        std::vector<uint32_t> work(n);

        std::iota(input.begin(), input.end(), 1);
        std::shuffle(input.begin(), input.end(), engine);

#pragma _NEC noinline
        int kth = radix_select(n, input.data(), work.data(), 8, k);

        std::cout << "n=" << n << " k=" << k << " kth=" << kth << std::endl;

        assert(k == kth);
    }

    return 0;
}
