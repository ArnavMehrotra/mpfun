#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <vector>

typedef struct LPCData {
    std::vector<float> _residuals;
    std::vector<float> _coefficients;
}

int huffmanCode(int x, int y);

#endif