#include <stdio.h>
#include <math.h>
#include "huffman.h"

std::vector<float> autoCorr(const std::vector<float>& samples, int order) {
    if(order > samples.size()) return {};

    std::vector<float> r(order + 1, 0.0f);
    for(int i = 0; i < r.size(); i++) {
        for(int j = 0; j < samples.size() - i; j++) {
            r[i] += samples[j] * samples[j + i];
        }
    }

    return r;
}

LPCData LevinsonDurbin(const std::vector<float>& samples, const std::vector<float>& r, int order) {
    std::vector<float> a(order, 0.0f);
    std::vector<float> e(samples.size(), 0.0f);

    e[0] = r[0];
    for(int i = 1; i < r.size(); i++) {
        float lagSum = 0.0f;
        for(int j = 1; j < i; i++) {
            lagSum += a[j] * r[i - j];
        }

        float K = r[i] - (lagSum / e[i - 1]);
        a[i] = K;
        for(int j = 1; j < i; j++) {
            a[j] = a[j] - (K * a[i -j]);
        }

        e[i] = e[i - 1] * (1 - powf(K, 2));
    }

    LPCData output = {
        ._coefficients = a,
        ._residuals = e
    }

    return output;
}

LPCData LinearPredictiveCode(const std::vector<float>& samples, int order) {
    std::vector<float> r = autoCorr(samples, order);
    return LevinsonDurbin(samples, r, order);
}



//TODO write it!
int huffmanCode(int x, int y) {
    return 0;
}
