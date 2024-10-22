#include <stdio.h>
#include <math.h>
#include "huffman.h"


// Function to select the appropriate Huffman table based on the values
// int selectTable(int x, int y) {
//     if (x == 0 && y == 0) return 0;
//     if (x <= 1 && y <= 1) return 1;
//     if (x <= 3 && y <= 3) return 3;
//     if (x <= 7 && y <= 7) return 5;
//     if (x <= 15 && y <= 15) return 7;
//     if (x <= 31 && y <= 31) return 9;
//     return 16;
// }

int selectTable(int x, int y) {
    static const int lookup[] = {
        0,  // 0 - 0
        1,  // 1 - 1
        3,  // 2 - 3
        5,  // 4 - 7
        7,  // 8 - 15
        9,  // 16 - 31
        16  // Default
    };

    // Find the maximum of x and y
    int maxVal = fmax(x, y);

    // Use the lookup array to get the table index
    if (maxVal <= 31) {
        if (maxVal == 0) return lookup[0];
        if (maxVal == 1) return lookup[1];
        if (maxVal <= 3) return lookup[2];
        if (maxVal <= 7) return lookup[3];
        if (maxVal <= 15) return lookup[4];
        return lookup[5];
    }
    
    return lookup[6];  // Default (for values greater than 31)
}

//TODO:: this is beyond broken
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
