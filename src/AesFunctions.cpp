#include "Aes.h"

Aes::Aes(){}

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

void Aes::hash_to_16bytes(const char* input, uint8_t output[16]) {
    uint8_t hash[SHA256_DIGEST_LENGTH];
    SHA256((const uint8_t*)input, strlen(input), hash);
    memcpy(output, hash, 16);
}
