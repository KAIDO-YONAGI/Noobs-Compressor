#include "../include/My_Aes.h"
#include <cstring>

// 轻量级 SHA256 实现
namespace {
    #define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
    #define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
    #define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
    #define EP0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
    #define EP1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
    #define SIG0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
    #define SIG1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

    const uint32_t K[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    void sha256_compress(uint32_t state[8], const uint8_t* data) {
        uint32_t W[64];
        for (int i = 0; i < 16; i++) {
            W[i] = (data[i*4] << 24) | (data[i*4+1] << 16) | (data[i*4+2] << 8) | data[i*4+3];
        }
        for (int i = 16; i < 64; i++) {
            W[i] = SIG1(W[i-2]) + W[i-7] + SIG0(W[i-15]) + W[i-16];
        }
        uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
        uint32_t e = state[4], f = state[5], g = state[6], h = state[7];
        for (int i = 0; i < 64; i++) {
            uint32_t t1 = h + EP1(e) + CH(e, f, g) + K[i] + W[i];
            uint32_t t2 = EP0(a) + MAJ(a, b, c);
            h = g; g = f; f = e; e = d + t1;
            d = c; c = b; b = a; a = t1 + t2;
        }
        state[0] += a; state[1] += b; state[2] += c; state[3] += d;
        state[4] += e; state[5] += f; state[6] += g; state[7] += h;
    }

    void sha256(const uint8_t* input, size_t len, uint8_t* output) {
        uint32_t state[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                            0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
        size_t blocks = (len + 9 + 63) / 64;
        uint8_t* padded = new uint8_t[blocks * 64];
        memcpy(padded, input, len);
        padded[len] = 0x80;
        memset(padded + len + 1, 0, blocks * 64 - len - 1);
        uint64_t bitlen = len * 8;
        for (int i = 0; i < 8; i++) {
            padded[blocks * 64 - 1 - i] = (bitlen >> (i * 8)) & 0xff;
        }
        for (size_t i = 0; i < blocks; i++) {
            sha256_compress(state, padded + i * 64);
        }
        for (int i = 0; i < 8; i++) {
            output[i*4] = (state[i] >> 24) & 0xff;
            output[i*4+1] = (state[i] >> 16) & 0xff;
            output[i*4+2] = (state[i] >> 8) & 0xff;
            output[i*4+3] = state[i] & 0xff;
        }
        delete[] padded;
    }
}

int Aes::getLeft4Bit(int num) {
    int left = num & 0x000000f0;
    return left >> 4;
}

int Aes::getRight4Bit(int num) {
    return num & 0x0000000f;
}

int Aes::getNumFromSBox(int index) {
    int row = getLeft4Bit(index);
    int col = getRight4Bit(index);
    return S[row][col];
}

int Aes::getIntFromChar(char c) {
    int result = (int)c;
    return result & 0x000000ff;
}

void Aes::convertToIntArray(char* str, int pa[4][4]) {
    int k = 0;
    for(int i = 0; i < 4; i++)
        for(int j = 0; j < 4; j++) {
            pa[j][i] = getIntFromChar(str[k]);
            k++;
        }
}

int Aes::getWordFromStr(const char* str) {
    int one = getIntFromChar(str[0]);
    one = one << 24;
    int two = getIntFromChar(str[1]);
    two = two << 16;
    int three = getIntFromChar(str[2]);
    three = three << 8;
    int four = getIntFromChar(str[3]);
    return one | two | three | four;
}

void Aes::splitIntToArray(int num, int array[4]) {
    int one = num >> 24;
    array[0] = one & 0x000000ff;
    int two = num >> 16;
    array[1] = two & 0x000000ff;
    int three = num >> 8;
    array[2] = three & 0x000000ff;
    array[3] = num & 0x000000ff;
}

void Aes::leftLoop4int(int array[4], int step) {
    int temp[4];
    for(int i = 0; i < 4; i++)
        temp[i] = array[i];

    int index = step % 4 == 0 ? 0 : step % 4;
    for(int i = 0; i < 4; i++) {
        array[i] = temp[index];
        index++;
        index = index % 4;
    }
}

int Aes::mergeArrayToInt(int array[4]) {
    int one = array[0] << 24;
    int two = array[1] << 16;
    int three = array[2] << 8;
    int four = array[3];
    return one | two | three | four;
}

int Aes::T(int num, int round) {
    int numArray[4];
    splitIntToArray(num, numArray);
    leftLoop4int(numArray, 1);

    for(int i = 0; i < 4; i++)
        numArray[i] = getNumFromSBox(numArray[i]);

    return mergeArrayToInt(numArray) ^ Rcon[round];
}

void Aes::extendKey(const char* key) {
    for(int i = 0; i < 4; i++)
        w[i] = getWordFromStr(key + i * 4);

    for(int i = 4, j = 0; i < 44; i++) {
        if(i % 4 == 0) {
            w[i] = w[i - 4] ^ T(w[i - 1], j);
            j++;
        }
        else {
            w[i] = w[i - 4] ^ w[i - 1];
        }
    }
}

void Aes::addRoundKey(int array[4][4], int round) {
    int warray[4];
    for(int i = 0; i < 4; i++) {
        splitIntToArray(w[round * 4 + i], warray);
        for(int j = 0; j < 4; j++) {
            array[j][i] = array[j][i] ^ warray[j];
        }
    }
}

void Aes::subBytes(int array[4][4]) {
    for(int i = 0; i < 4; i++)
        for(int j = 0; j < 4; j++)
            array[i][j] = getNumFromSBox(array[i][j]);
}

void Aes::shiftRows(int array[4][4]) {
    int rowTwo[4], rowThree[4], rowFour[4];
    for(int i = 0; i < 4; i++) {
        rowTwo[i] = array[1][i];
        rowThree[i] = array[2][i];
        rowFour[i] = array[3][i];
    }

    leftLoop4int(rowTwo, 1);
    leftLoop4int(rowThree, 2);
    leftLoop4int(rowFour, 3);

    for(int i = 0; i < 4; i++) {
        array[1][i] = rowTwo[i];
        array[2][i] = rowThree[i];
        array[3][i] = rowFour[i];
    }
}

void Aes::mixColumns(int array[4][4]) {
    int tempArray[4][4];
    memcpy(tempArray, array, sizeof(tempArray));

    for(int i = 0; i < 4; i++) {
        array[i][0] = GF_MUL_2[tempArray[i][0]] ^ GF_MUL_3[tempArray[i][1]] ^ tempArray[i][2] ^ tempArray[i][3];
        array[i][1] = tempArray[i][0] ^ GF_MUL_2[tempArray[i][1]] ^ GF_MUL_3[tempArray[i][2]] ^ tempArray[i][3];
        array[i][2] = tempArray[i][0] ^ tempArray[i][1] ^ GF_MUL_2[tempArray[i][2]] ^ GF_MUL_3[tempArray[i][3]];
        array[i][3] = GF_MUL_3[tempArray[i][0]] ^ tempArray[i][1] ^ tempArray[i][2] ^ GF_MUL_2[tempArray[i][3]];
    }
}

void Aes::convertArrayToStr(int array[4][4], char* str) {
    for(int i = 0; i < 4; i++)
        for(int j = 0; j < 4; j++)
            *str++ = (char)array[j][i];
}

int Aes::getNumFromS1Box(int index) {
    int row = getLeft4Bit(index);
    int col = getRight4Bit(index);
    return S2[row][col];
}

void Aes::deSubBytes(int array[4][4]) {
    for(int i = 0; i < 4; i++)
        for(int j = 0; j < 4; j++)
            array[i][j] = getNumFromS1Box(array[i][j]);
}

void Aes::rightLoop4int(int array[4], int step) {
    int temp[4];
    for(int i = 0; i < 4; i++)
        temp[i] = array[i];

    int index = step % 4 == 0 ? 0 : step % 4;
    index = 3 - index;
    for(int i = 3; i >= 0; i--) {
        array[i] = temp[index];
        index--;
        index = index == -1 ? 3 : index;
    }
}

void Aes::deShiftRows(int array[4][4]) {
    int rowTwo[4], rowThree[4], rowFour[4];
    for(int i = 0; i < 4; i++) {
        rowTwo[i] = array[1][i];
        rowThree[i] = array[2][i];
        rowFour[i] = array[3][i];
    }

    rightLoop4int(rowTwo, 1);
    rightLoop4int(rowThree, 2);
    rightLoop4int(rowFour, 3);

    for(int i = 0; i < 4; i++) {
        array[1][i] = rowTwo[i];
        array[2][i] = rowThree[i];
        array[3][i] = rowFour[i];
    }
}

void Aes::deMixColumns(int array[4][4]) {
    int tempArray[4][4];
    memcpy(tempArray, array, sizeof(tempArray));

    for(int i = 0; i < 4; i++) {
        array[i][0] = GF_MUL_14[tempArray[i][0]] ^ GF_MUL_11[tempArray[i][1]] ^ GF_MUL_13[tempArray[i][2]] ^ GF_MUL_9[tempArray[i][3]];
        array[i][1] = GF_MUL_9[tempArray[i][0]] ^ GF_MUL_14[tempArray[i][1]] ^ GF_MUL_11[tempArray[i][2]] ^ GF_MUL_13[tempArray[i][3]];
        array[i][2] = GF_MUL_13[tempArray[i][0]] ^ GF_MUL_9[tempArray[i][1]] ^ GF_MUL_14[tempArray[i][2]] ^ GF_MUL_11[tempArray[i][3]];
        array[i][3] = GF_MUL_11[tempArray[i][0]] ^ GF_MUL_13[tempArray[i][1]] ^ GF_MUL_9[tempArray[i][2]] ^ GF_MUL_14[tempArray[i][3]];
    }
}

void Aes::addRoundTowArray(int aArray[4][4], int bArray[4][4]) {
    for(int i = 0; i < 4; i++)
        for(int j = 0; j < 4; j++)
            aArray[i][j] = aArray[i][j] ^ bArray[i][j];
}

void Aes::getArrayFrom4W(int i, int array[4][4]) {
    int colOne[4], colTwo[4], colThree[4], colFour[4];
    int index = i * 4;
    splitIntToArray(w[index], colOne);
    splitIntToArray(w[index + 1], colTwo);
    splitIntToArray(w[index + 2], colThree);
    splitIntToArray(w[index + 3], colFour);

    for(int j = 0; j < 4; j++) {
        array[j][0] = colOne[j];
        array[j][1] = colTwo[j];
        array[j][2] = colThree[j];
        array[j][3] = colFour[j];
    }
}

void Aes::hash_to_16bytes(const char* input, uint8_t* output) {
    uint8_t hash[32];
    sha256((const uint8_t*)input, strlen(input), hash);
    memcpy(output, hash, 16);
}
