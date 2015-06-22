#include <stdio.h>
#include <stdlib.h>

typedef unsigned char byte;
typedef unsigned short hWord;   // half word

FILE *fp;
long offSet;
byte buff_block[32768];
byte buff_byte;
hWord FAT[32768];
hWord nByte = 512, nBlock;    // number of bytes within a block, number of blocks total
hWord nFAT, nBFAT; // number of FATs, number of blocks within a FAT

void readBlock(long addr) {    // addr: the address of blocks
    hWord offset;
    offset = addr * nByte;
    fseek(fp, offSet, SEEK_SET);
    fread(buff_block, 1, nByte, fp);
}

byte readByte(long addr) { // addr: the address of a specific byte
    fseek(fp, addr, SEEK_SET);
    fread(&buff_byte, 1, 1, fp);
    return buff_byte;
}

void readHWord(hWord *hw, long offset) {
    hWord tmp;
    tmp = readByte(offset + 1);
    tmp <<= 8;
    tmp += readByte(offset);
    *hw = (hWord)tmp;
}

void init() {
    fp = fopen("papapapa.vhd", "rb+");
    offSet = 0x1000b;   // jump to boot block
    readHWord(&nByte, offSet);
    offSet = 0x10010;
    readHWord(&nFAT, offSet);
    offSet = 0x10013;
    readHWord(&nBlock, offSet);
    offSet = 0x10016;
    readHWord(&nBFAT, offSet);
    printf("nByte: %u\n", nByte);
    printf("nFAT: %u\n", nFAT);
    printf("nBlock: %u\n", nBlock);
    printf("nBFAT: %u\n", nBFAT);
}

int main() {
    init();
}
