#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char byte;
typedef unsigned short hWord;   // half word
typedef unsigned long word;
typedef struct mytime{
    hWord year, mon, day, hour, min, sec;
} mTime;

typedef struct fileStruct{
    char name[9];
    char ext[4];
    char attr;
    mTime ftime;
    word start;
    word size;
} file;

FILE *fp;
long offSet;
byte buff_block[32768];
byte buff_byte;
hWord FAT[32768];
hWord nByte = 512, nCluster;    // number of bytes within a block, number of clusters on total
hWord nFAT, nBFAT; // number of FATs, number of blocks within a FAT
hWord nFile;    // number of files in root directory
file* fIndex[512];
const char* const month[13] = {"", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nev", "Dec"};

void readBlock(word addr) {    // addr: the address of blocks
    hWord offset;
    offset = addr * nByte;
    fseek(fp, offSet, SEEK_SET);
    fread(buff_block, 1, nByte, fp);
}

byte readByte(word addr) { // addr: the address of a specific byte
    fseek(fp, addr, SEEK_SET);
    fread(&buff_byte, 1, 1, fp);
    return buff_byte;
}

hWord readHWord(word offset) {
    return (readByte(offset + 1) << 8) + readByte(offset);
}

word readWord(word offset) {
    return (readHWord(offset + 2) << 16) + readHWord(offset);
}

void readFAT() {
    hWord i;
    for (i = 0; i < nCluster; i++) {
        FAT[i] = readHWord(offSet + i * 2);
    }
}

int readFileName(hWord n) {
    hWord i;
    byte head = readByte(offSet + n * 32);
    if ((head == 0xE5) || (head == 0x00))
        return 0;
    fIndex[n] = (file*)malloc(sizeof(file));
    for (i = 0; i < 8; i++)
        fIndex[n]->name[i] = readByte(offSet + n * 32 + i);
    for (i = 7; i >= 0; i--) {
        if (fIndex[n]->name[i] != 32)
            break;
        fIndex[n]->name[i] = '\0';
    }
    return 1;
}

void readFileExt(hWord n) {
    hWord i;
    for (i = 0; i < 3; i++)
        fIndex[n]->ext[i] = readByte(offSet + n * 32 + 8 + i);
    for (i = 3; i >= 0; i--) {
        if (fIndex[n]->name[i] != 32)
            break;
        fIndex[n]->name[i] = '\0';
    }
}

void readFileAttr(hWord n) {
    fIndex[n]->attr = readByte(offSet + n * 32 + 11);
}

void readFileTimeStamp(hWord n) {
    hWord td;
    td = readHWord(offSet + n * 32 + 22);
    fIndex[n]->ftime.hour = td >> 11;
    fIndex[n]->ftime.min = (td >> 5) & 0x3f;
    fIndex[n]->ftime.sec = (td & 0x1f) << 1;
    td = readHWord(offSet + n * 32 + 24);
    fIndex[n]->ftime.year = (td >> 9) + 1980;
    fIndex[n]->ftime.mon = (td >> 5) & 0xf;
    fIndex[n]->ftime.day = td & 0x1f;
}

void readFileStart(hWord n) {
    fIndex[n]->start = readHWord(offSet + n * 32 + 26);
}

void readFileSize(hWord n) {
    fIndex[n]->size = readWord(offSet + n * 32 + 28);
}

void readRD() {
    hWord i;
    for (i = 0; i < nFile; i++) {
        if (readFileName(i) > 0) {
            readFileExt(i);
            readFileAttr(i);
            readFileTimeStamp(i);
            readFileStart(i);
            readFileSize(i);
        }
    }
}

void printFileName(hWord n) {
    hWord i;
    printf("%s.%s", fIndex[n]->name, fIndex[n]->ext);
    for (i = 0; i < 15 - strlen(fIndex[n]->name) - strlen(fIndex[n]->ext); i++)
        printf(" ");
}

void printFileAttr(hWord n) {
    hWord i, pt = 0x80;
    for (i = 0; i < 8; i++) {
        if ((fIndex[n]->attr & pt) == 0)
            printf("0");
        else
            printf("1");
        pt >>= 1;
    }
    printf("  ");
}

void printFileTime(hWord n) {
    // print time
    if (fIndex[n]->ftime.hour < 10)
        printf("0");
    printf("%u:", fIndex[n]->ftime.hour);
    if (fIndex[n]->ftime.min < 10)
        printf("0");
    printf("%u:", fIndex[n]->ftime.min);
    if (fIndex[n]->ftime.sec < 10)
        printf("0");
    printf("%u  ", fIndex[n]->ftime.sec);

    // print date
    printf("%2u %3s %4u  ", fIndex[n]->ftime.day, month[fIndex[n]->ftime.mon], fIndex[n]->ftime.year);
}

void printFileSize(hWord n) {
    printf("%12lu\n", fIndex[n]->size);
}

void init() {
    fp = fopen("papapapa.vhd", "rb+");
    offSet = 0x1000b;   // jump to boot block
    nByte = readHWord(offSet);
    offSet = 0x10010;
    nFAT = (hWord)readByte(offSet);
    offSet = 0x10011;
    nFile = readHWord(offSet);
    offSet = 0x10013;
    nCluster = readHWord(offSet);
    offSet = 0x10016;
    nBFAT = readHWord(offSet);
/*    printf("nByte: %u\n", nByte);
    printf("nFAT: %u\n", nFAT);
    printf("nCluster: %u\n", nCluster);
    printf("nBFAT: %u\n", nBFAT);*/
    offSet = 0x10400;
    readFAT();
    offSet += nFAT * nBFAT * nByte;
    readRD();   // read root directory
}

void ls() {
    word i, j;
    printf("        id     |  file name   |  Attr   |      timestamp       | size    \n");
    printf("--------------------------------------------------------------------------\n");
    for (i = 0; i < nFile; i++) {
        if (fIndex[i]) {
            if (((fIndex[i]->attr & 8) == 0) & ((fIndex[i]->attr & 4) == 0) & ((fIndex[i]->attr & 2) == 0)) {
                printf("%12lu    ", i);
                printFileName(i);
                printFileAttr(i);
                printFileTime(i);
                printFileSize(i);
            }
        }
    }
}

int main() {
    int i;
    init();
    ls();
}
