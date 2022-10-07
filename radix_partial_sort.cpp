#include <algorithm>
#include <cassert>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

const int VLEN = 256;

void counting_sort(int n, uint32_t *v, uint32_t *r, int bits, int pos, int k,
                   int *new_n, uint32_t **new_v, uint32_t **new_r, int *new_k)
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
    *new_v = v + bucket[nb - 1][0];
    *new_r = r + bucket[nb - 1][0];
    *new_n = n - bucket[nb - 1][0];

    for (int i = 0; i < nb; i++) {
        if (k <= bucket[i][0]) {
            *new_k = k - bucket[i - 1][0];
            *new_v = v + bucket[i - 1][0];
            *new_r = r + bucket[i - 1][0];
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
    uint32_t *new_v, *new_r;

    for (int pos = (sizeof(uint32_t) * 8) / bits - 1; pos >= 0; pos--) {
        counting_sort(n, v, r, bits, pos, k, &new_n, &new_v, &new_r, &new_k);

        for (int i = 0; i < n; i++) {
            v[i] = r[i];
        }

        n = new_n;
        v = new_v;
        r = new_r;
        k = new_k;

        if (n == 1) break;
    }

    return v[0];
}

void radix_partial_sort(int n, float *v, float *w, int *ix, int k)
{
    uint32_t kth = radix_select(n, reinterpret_cast<uint32_t *>(v),
                                reinterpret_cast<uint32_t *>(w), 8, k);

    int cnt = 0;
    for (int i = 0; i < n; i++) {
        if (v[i] <= *reinterpret_cast<float *>(&kth)) {
            ix[cnt] = i;
            cnt++;
        }
    }
}

int main()
{
    std::mt19937 engine(42);
    std::uniform_int_distribution<int> dist(1, 100);

    for (int iter = 0; iter < 100; iter++) {
        int n = dist(engine);

        std::uniform_int_distribution<int> dist2(1, n);

        int k = dist2(engine);

        std::vector<float> input(n);
        std::vector<float> work(n);
        std::vector<int> index(k);

        std::uniform_real_distribution<float> dist3;

        std::cout << "n=" << n << " k=" << k << std::endl;

        for (auto &elem : input) {
            elem = dist3(engine);
        }

        uint32_t tmp =
            radix_select(n, reinterpret_cast<uint32_t *>(input.data()),
                         reinterpret_cast<uint32_t *>(work.data()), 8, k);
        float kth_actual = *reinterpret_cast<float *>(&tmp);

        std::nth_element(input.begin(), input.begin() + (k - 1), input.end());
        float kth_correct = input[k - 1];

        std::cout << "kth_actual=" << kth_actual
                  << " kth_correct=" << kth_correct << std::endl;

        assert(kth_actual == kth_correct);
    }

    return 0;
}
