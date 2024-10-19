#include <stdio.h>
#include "huffman.h"

// Function to select the appropriate Huffman table based on the values
int selectTable(int x, int y) {
    if (x == 0 && y == 0) return 0;
    if (x <= 1 && y <= 1) return 1;
    if (x <= 3 && y <= 3) return 3;
    if (x <= 7 && y <= 7) return 5;
    if (x <= 15 && y <= 15) return 7;
    if (x <= 31 && y <= 31) return 9;
    return 16;
}


int huffmanCode(int x, int y) {
    x = abs(x);
    y = abs(y);

    int tableIndex = selectTable(x, y);
    const short* table = (&ht[tableIndex])->table;

    int index = 0;
    while(table[index] < 0) {
        if(x > (table[index] * -1)) {
            index = -table[index];
        } else {
            index++;
        }
    }

    int code = table[index];

    //TODO:: handle linbits

    return code;
}
