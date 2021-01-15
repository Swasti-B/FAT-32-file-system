# FAT-32 file system. Operating system assignment assigned by: Professor Trevor Bakker

The program is written in C and has the implementation of a user space shell application that is capable of interpreting a FAT32 file system image. 
It supports/performs the following commands on a FAT32 image:
1. Open <filename>
2. close
3. info
4. stat <filename> or <directory name>
5. get <filename>
6. cd <directory>
7. ls
8. read <filename> <position> <number of bytes>
 
 Instruction for compilation: g++ mfs.c -o mfs
                              mfs
 
 
