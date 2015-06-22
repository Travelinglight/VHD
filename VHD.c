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
    hWord start;
    word size;
} file;

FILE *fp;
word offSet;
byte buff_block[32768];
byte buff_byte;
hWord FAT[32768];
hWord nByte = 512, nCluster;    // number of bytes within a block, number of clusters on total
hWord nFAT, nBFAT; // number of FATs, number of blocks within a FAT
hWord nFile, nBlock;    // number of files in root directory, number of blocks in a cluster
file* fIndex[512];
const char* const month[13] = {"", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nev", "Dec"};

void readBlock(word addr) {    // addr: the address of blocks
    fseek(fp, addr, SEEK_SET);
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

void cpCluster(FILE *out, hWord cp1) {  // function copy, cp1: cluster pointer 1
    hWord i;
    for (i = 0; i < nBlock; i++) {
        readBlock(offSet + ((cp1 - 2) * nBlock + i) * nByte);
        fwrite(buff_block, 1, nByte, out);
    }
}

void init() {
    fp = fopen("papapapa.vhd", "rb+");
    offSet = 0x1000b;   // jump to boot block
    nByte = readHWord(offSet);
    offSet = 0x1000d;
    nBlock = readByte(offSet);
    offSet = 0x10010;
    nFAT = (hWord)readByte(offSet);
    offSet = 0x10011;
    nFile = readHWord(offSet);
    offSet = 0x10013;
    nCluster = readHWord(offSet);
    offSet = 0x10016;
    nBFAT = readHWord(offSet);
    offSet = 0x10400;
    readFAT();
    offSet += nFAT * nBFAT * nByte;
    readRD();   // read root directory
    offSet += nFile * 32;
}

void ls() {
    hWord i, j;
    printf("        id     |  file name   |  Attr   |      timestamp       | size    \n");
    printf("--------------------------------------------------------------------------\n");
    for (i = 0; i < nFile; i++) {
        if (fIndex[i]) {
            if (((fIndex[i]->attr & 8) == 0) & ((fIndex[i]->attr & 4) == 0) & ((fIndex[i]->attr & 2) == 0)) {
                printf("%12u    ", i);
                printFileName(i);
                printFileAttr(i);
                printFileTime(i);
                printFileSize(i);
            }
        }
    }
}

void cp(hWord n) {
    char fullName[15];
    hWord cp1 = 0;
    if (!fIndex[n])
        return;
    strcpy(fullName, fIndex[n]->name);
    strcat(fullName, ".");
    strcat(fullName, fIndex[n]->ext);
    FILE *out;
    out = fopen(fullName, "wb");
    cp1 = fIndex[n]->start;
    while (cp1 != 0xFFFF) {
        cpCluster(out, cp1);
        cp1 = FAT[cp1];
    }
    fclose(out);
}

void rm(int n) {
    hWord i, j;
    hWord cp1 = 0, cp2 = 0;
    if (fIndex[n] == NULL) {
        printf("file not exists!\n");
        return;
    }
    cp1 = fIndex[n]->start;
    while (cp1 != 0xFFFF) {
        cp2 = FAT[cp1];
        FAT[cp1] = 0x0000;
        for (i = 0; i < nFAT; i++) {
            fseek(fp, 0x10400 + i * (nBFAT * nByte) + cp1 * 2, SEEK_SET);
            fputc(0, fp);
            fputc(0, fp);
        }
        cp1 = cp2;
    }
    fseek(fp, offSet - nFile * 32 + n * 32, SEEK_SET);
    fputc(0xE5, fp);
    free(fIndex[n]);
    fIndex[n] = NULL;
    printf("Done\n");
}

int main() {
    init();
    ls();
    cp(3);
    rm(3);
    ls();
}
