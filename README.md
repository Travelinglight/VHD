# VHD

## Introduction
The project VHD is going to implement basic functions to handle files in Virtual FAT32 disk.

## Environment
1. OS: OS X Yosemite Version 10.10.3
2. language: C
3. compiler: gcc

## Installation & Usage
1. clone the repo
  
  ~~~
  git clone git@github.com:Travelinglight/VHD.git
  ~~~
2. use Windows to generate a .vhd file
  * the format of the file should be FunAT32
  * the filesize should not be too large; 20MB is recommended
  * the number of bytes per block is recommended to be 512
  * the .vhd file should be put into the repo

3. compile VHD.c

  ~~~
  gcc -g -o VHD VHD.c
  ~~~
  
4. run the executable file, and follow the instructions:

  ~~~
  ./VHD
  ~~~
  type "help" for instructions list
  
## Things you can do
1. list out the file list, with detailed infomation
2. remove a file from the VHD
3. copy a file from the VHD to the outside
4. move a file from outside into the VHD

## Variables Specification
1. FILE* fp ---- file point for .vhd file
2. word offSet ---- the offset of fp
3. byte buff_block[32768] ---- the buff of a block
4. byte buff_byte ---- the buff of a byte
5. hWord FAT[32768] ---- the FAT store in memory
6. hWord nByte ---- the number of bytes within a block
7. hWord nBlock ---- the number of blocks within a cluster
8. hWord nCluster ---- the number of clusters in data area of the vhd
9. hWord nFAT ---- the number of FATs in the vhd
10. hWord nBFAT ---- the number of blocks within a FAT
11. hWord nFile ---- the maximum number of files that could be recorded in the root directory
12. file* fIndex[512] ---- the file index maintained in memory, corresponding to the root directory

## Functions Specification
### Important functions
1. <b>init</b>
  1. read parameters from boot block
  2. read the whole FAT
  3. read root directory and initialize the fIndex array
2. <b>ls: list file infomation</b>
  1. list the file id, file name, attribute, timestamp, and file size for each file in the fIndex
3. <b>cp: copy a file from vhd to outside</b>
  1. generate the filename of the outside file, and open it
  2. read data area according to the FAT, and write to the destination file block by block
4. <b>rm: remove a file from VHD</b>
  1. clear the FAT chain (reset those half_word in FAT describing the clusters of the file to 0)
  2. put 0xE5 to the first byte of the filename in root directory
  3. free and clear the file struct in the fIndex array
5. <b>mv: move a file from outside into the VHD</b>
  1. parse the filename and file_extention
  2. get the file size
  3. find and clear enough clusters that are not used, and record them in an array
  4. obtain other necessary attributes describing a file
  5. modify root directory with the attributes obtained before
  6. modify FAT according to the clusters array found out before
  7. read binary data from source file, and write them into the clusters found before, byte by byte

### Complementary functions
1. readBlock: to read a block from .vhd file into buff_block
2. readByte: to read and return a byte from .vhd file
3. readHWord: to read and return a half_word from .vhd file. This function calls:
  * readByte;
4. readWord: to read and return a word from .vhd file. This function calls:
  * readHWord;
5. readFAT: to read the while FAT from .vhd file into FAT
6. raedFileName: to read the filename from .vhd file, format it and handle special cases. This function calls:
  * readByte;
7. readFileExt: to read the file extention from .vhd file and format it. This function calls:
  * readByte;
8. readAttr: to read the attribute of a file from .vhd file. This function calls:
  * readByte;
9. readFileTimeStamp: to read the time and date of the file from root directory in .vhd file. This function calls:
  * readHWord;
10. readFileStart: to read the id of start cluster of the file from .vhd file. This function calls:
  * readHWord;
11. readFileSize: to read the file size from the root directory in .vhd file. This function calls:
  * readHWord;
12. readRD: to read the info of all files. This function calls:
  * readFileExt;
  * readFileAttr;
  * readFileTimeStamp;
  * readFileStart;
  * readFileSize;
13. printFileName: to print the filename in normal way
14. printFileAttr: to print the file attribute in binary format
15. printFileTime: to print the time of the file vividly
16. printFileSize: to print the file size in Bytes
17. cpCluster: to copy a whole cluster to destination field, which is called by function cp;
18. cvt2upper: to convert a string (e.g. filename) to uppercase string
19. getTime: to get the current time and transform it and store it in a struct
20. writeTime: to write the time and date into the root directory in .vhd file

## Existing bugs
1. After a .txt file is written into .vhd file, there would be an extra half_word (0x0AFF) appended to the original file.
2. The file of 2 bytes would become 512 bytes after being written into and extracted out from VHD.
3. Some constants are used in the program, so the program may not be universal to all FAT32 VHDs.
