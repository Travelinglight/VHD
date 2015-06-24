#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
boot addr:     10000
fat addr:      10400
root dic addr: 32000
data cluster:  36000
*/

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
hWord FAT[32768];   // [0][1] reserved  start from [2][3]
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
    int i;
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
    int i;
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

void cvt2upper(char *str) {
    int i;
    for (i = 0; i < strlen(str); i++) {
        if ((str[i] >= 97) && (str[i] <= 122))
            str[i] -= 32;
    }
}

void getTime(mTime *ftime) {
    time_t timep;
    struct tm *p;
    time(&timep);
    p = localtime(&timep);
    ftime->year = p->tm_year + 1900;
    ftime->mon = p->tm_mon + 1;
    ftime->day = p->tm_mday;
    ftime->hour = p->tm_hour;
    ftime->min = p->tm_min;
    ftime->sec = p->tm_sec;
}

void writeTime(mTime ftime) {
    hWord td;
    td = (ftime.hour << 11) + (ftime.min << 5) + (ftime.sec >> 1);
    fputc((byte)(td & 0xFF),fp);
    fputc((byte)((td >> 8) & 0xFF),fp);
    td = ((ftime.year - 1980) << 9) + (ftime.mon << 5) + ftime.day;
    fputc((byte)(td & 0xFF),fp);
    fputc((byte)((td >> 8) & 0xFF),fp);
}

void init(char *vhdName) {
    fp = fopen(vhdName, "rb+");
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

void rm(hWord n) {
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

void mv(char *filename){
    FILE *p;
    word offset;
    hWord linkedCluster[32676];
    int i, j, k;
    int fileNumber;
    byte tmp;
    file *newFile;

    if(strchr(filename, '.') - filename <= 8){
        // deal with name and extent
        newFile = (file*)malloc(sizeof(file));
        strncpy(newFile->name, filename, strchr(filename, '.') - filename);
        cvt2upper(newFile->name);
        for (i = strlen(newFile->name); i < 8; i++)
            newFile->name[i] = ' ';
        newFile->name[8] = '\0';
        strcpy(newFile->ext, strchr(filename, '.') + 1);
        cvt2upper(newFile->ext);
        for (i = strlen(newFile->ext); i < 4; i++)
            newFile->ext[i] = '\0';
        printf("%s %s\n", newFile->name, newFile->ext);
        
        // get size
        p = fopen(filename, "rb");//open target file
        fseek(p, 0, SEEK_END);
        newFile->size = ftell(p);//the size of the target file
        fseek(p, 0, SEEK_SET);
        
        // get the clusters to place the file
        linkedCluster[0] = (newFile->size - 1) / (nByte * nBlock) + 1;// a cluster occupies 512 bytes
        for(i = 2, j = 1; i < nBFAT*nByte && j <= linkedCluster[0]; ++i)
            if(FAT[i] == 0)
                linkedCluster[j++] = i;
        linkedCluster[j] = 0xFFFF;
        newFile->start = linkedCluster[1];

        // set the attribute
        newFile->attr = 0x20;

        // update file index list
        fileNumber = -1;
        while(fIndex[++fileNumber]);
        fIndex[fileNumber] = newFile;

        // modify root directory
        offset = 0x32000 + fileNumber * 0x20;
        fseek(fp,offset,SEEK_SET);
        for(i = 0; i < 8; ++i)
            fputc((byte)newFile->name[i],fp);
        for(i = 0; i < 3; ++i)
            fputc((byte)newFile->ext[i],fp);
        fputc(0x20,fp); // file attr
        fseek(fp,10,SEEK_CUR);// escape 10 reserved bytes
/*        for(i = 0; i < 4; ++i)//file time and date
            fputc((byte)(10),fp);*/
        getTime(&(newFile->ftime));
        writeTime(newFile->ftime);
        fputc((byte)(linkedCluster[1]%256),fp);// first cluster
        fputc((byte)(linkedCluster[1]/256),fp);
        fputc((byte)((newFile->size) & 0xFF),fp);//file size
        fputc((byte)((newFile->size >> 8) & 0xFF),fp);
        fputc((byte)((newFile->size >> 16) & 0xFF),fp);
        fputc((byte)((newFile->size >> 24) & 0xFF),fp);

        // modify FAT
        for(i = 1; i <= linkedCluster[0]; ++i){
            FAT[linkedCluster[i]] = linkedCluster[i + 1];
            offset = 0x10400 + linkedCluster[i] * 2;
            fseek(fp, offset, SEEK_SET);
            fputc((byte)(linkedCluster[i + 1] & 0xFF), fp);
            fputc((byte)(linkedCluster[i + 1] >> 8),fp);
        }
        
        //data block
        i=0;
        j=511;
        while(!feof(p)){
            if(++j == nByte * nBlock){ // one cluster occupies 1 block
                ++i;
                j = 0;
                offset = 0x36000 + (linkedCluster[i] - 2) * (nByte * nBlock);
                fseek(fp, offset, SEEK_SET);
                for (k = 0; k < nByte * nBlock; k++)
                    fputc(0, fp);
                fseek(fp, offset, SEEK_SET);
            }
            tmp = fgetc(p);
            fputc((byte)(tmp),fp);
        }

        newFile->name[strchr(filename, '.') - filename] = '\0';
    }else{
        printf("Invalaid filename.");
    }
}

int main() {
    char vhdname[16], filename[16];
    char cmd[20];
    int id;
    printf("Please input the name of VHD:\n");
    scanf("%s", vhdname);
    init(vhdname);
    while (1) {
        printf("What do you want to do?\n");
        scanf("%s", cmd);
        if (strcmp(cmd, "help") == 0) {
            printf("ls -- list out files and info\n");
            printf("cp -- copy a file from vhd out\n");
            printf("mv -- move a file from outside into vhd\n");
            printf("rm -- remove a file from vhd\n");
            printf("exit -- exit the program\n");
        }
        else if (strcmp(cmd, "ls") == 0) {
            ls();
        }
        else if (strcmp(cmd, "cp") == 0) {
            printf("Please input the file id u want to copy out:\n");
            scanf("%d", &id);
            cp(id);
        }
        else if (strcmp(cmd, "mv") == 0) {
            printf("Please: input the file you want to add into the visual disc.\nFilename: ");
            scanf("%s",filename);
            mv(filename);
        }
        else if (strcmp(cmd, "rm") == 0) {
            printf("Please input the file id you want to remove:\n");
            scanf("%d", &id);
            rm(id);
        }
        else if (strcmp(cmd, "exit") == 0) {
            exit(0);
        }
        else {
            printf("Please input the right command, input 'help' for more infomation.\n");
        }
    }
}
