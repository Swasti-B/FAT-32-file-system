// The MIT License (MIT)
//
// Copyright (c) 2016, 2017 Trevor Bakker
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include <ctype.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 5     // Mav shell only supports five arguments

uint16_t BPB_BytesPerSec;
uint8_t BPB_SecPerClus;
uint16_t BPB_RsvdSecCnt;
uint8_t BPB_NumFATs;
uint16_t BPB_RootEntCnt;
uint32_t BPB_FATSz32;

struct __attribute__((__packed__)) DirectoryEntry
{
char DIR_Name[11];
uint8_t Dir_Attr;
uint8_t Unused1[8];
uint16_t DIR_FirstClusterHigh;
uint8_t Unused[4];
uint16_t DIR_FirstClusterLow;
uint32_t DIR_FileSize;
};
struct DirectoryEntry dir[16];

int file_compare(char *file_DIRName)
{
    int position=-1;
    //printf("from compare: %s\n",file_DIRName);
    char expanded_name[12];
    memset( expanded_name, ' ',12);
    //printf("test: %d\n",strlen(file_DIRName));
    char *token = strtok(file_DIRName, "." );

    strncpy( expanded_name, token, strlen( token ) );

    token = strtok( NULL, "." );

    if( token )
    {
      strncpy( (char*)(expanded_name+8), token, strlen(token ) );
    }

    expanded_name[11] = '\0';

    int i;
    for( i = 0; i < 11; i++ )
    {
      expanded_name[i] = toupper( expanded_name[i] );
    }

    for( i = 0; i < 16; i++ )
    {
    if( strncmp( expanded_name, dir[i].DIR_Name, 11 ) == 0 )
    {
      position =i;
    }
    //printf("%d",dir[i].DIR_FileSize);
    //printf("\n");
  }
  return position;

}
int LBAToOffset(int32_t sector)
{
  return((sector-2)*BPB_BytesPerSec)+(BPB_NumFATs*BPB_FATSz32*BPB_BytesPerSec)+(BPB_RsvdSecCnt*BPB_BytesPerSec);
}

void info_command()
{
  printf("BPB_BytesPerSec: %d\n",BPB_BytesPerSec);
  printf("BPB_BytesPerSec: %x\n\n",BPB_BytesPerSec);

  printf("BPB_SecPerClus: %d\n",BPB_SecPerClus);
  printf("BPB_SecPerClus: %x\n\n",BPB_SecPerClus);

  printf("BPB_RsvdSecCnt: %d\n",BPB_RsvdSecCnt);
  printf("BPB_RsvdSecCnt: %x\n\n",BPB_RsvdSecCnt);

  printf("BPB_NumFATs : %d\n",BPB_NumFATs);
  printf("BPB_NumFATs : %x\n\n",BPB_NumFATs);

  printf("BPB_FATSz32 : %d\n",BPB_FATSz32);
  printf("BPB_FATSz32 : %x\n\n",BPB_FATSz32);
}


int main()
{

  char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );
  int count=0;
  char filename[100];

  while( 1 )
  {
    // Print out the mfs prompt
    printf ("mfs> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    int   token_count = 0;

    // Pointer to point to the token
    // parsed by strsep
    char *arg_ptr;

    char *working_str  = strdup( cmd_str );

    // we are going to move the working_str pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *working_root = working_str;

    // Tokenize the input stringswith whitespace used as the delimiter
    while ( ( (arg_ptr = strsep(&working_str, WHITESPACE ) ) != NULL) &&
              (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL;
      }
        token_count++;
    }

    // Now print the tokenized input as a debug check
    // \TODO Remove this code and replace with your shell functionality

    /*int token_index  = 0;
    for( token_index = 0; token_index < token_count; token_index ++ )
    {
      printf("token[%d] = %s\n", token_index, token[token_index] );
    }
    */


    FILE *ptr;
    if(strcmp(token[0],"open")==0)
    {
      count++;
      if(count>1)
      {
        printf("Error: File system image already open.\n");
        count--;
        continue;
      }
      ptr=fopen(token[1],"r");
      if(!ptr)
      {
      printf("Error: Could not open the file.\n");
      count--;
      continue;
      }
      strcpy(filename,token[1]);

      fseek(ptr,11,SEEK_SET);
      fread(&BPB_BytesPerSec,2,1,ptr);

      fseek(ptr,13,SEEK_SET);
      fread(&BPB_SecPerClus,1,1,ptr);

      fseek(ptr,14,SEEK_SET);
      fread(&BPB_RsvdSecCnt,2,1,ptr);

      fseek(ptr,16,SEEK_SET);
	    fread(&BPB_NumFATs,1,1,ptr);

      fseek(ptr,36,SEEK_SET);
	    fread(&BPB_FATSz32,4,1,ptr);

      int root_cluster=(BPB_NumFATs*BPB_FATSz32*BPB_BytesPerSec)+(BPB_RsvdSecCnt*BPB_BytesPerSec);
      fseek(ptr,root_cluster,SEEK_SET);
      fread(&dir[0],16,sizeof(struct DirectoryEntry),ptr);
      continue;
    }
    else if(strcmp(token[0],"info")==0)
    {
      if(count==0)
      {
        printf("Error: File system image must be opened first.\n");
        continue;
      }
      info_command();
      continue;
    }
    else if(strcmp(token[0],"stat")==0)
    {
      if(count==0)
      {
        printf("Error: File system image must be opened first.\n");
        continue;
      }
      else if(token[1]=='\0')
      {
        printf("Error: File not found.\n");
        continue;
      }
      int check_inputFile=file_compare(token[1]);
      if(check_inputFile!=-1)
      {
        printf("Attribute\tSize\tStarting Cluster Number\n");
        printf("%d\t\t%d\t%d\n",dir[check_inputFile].Dir_Attr,dir[check_inputFile].DIR_FileSize,dir[check_inputFile].DIR_FirstClusterLow);
      }
      else if(check_inputFile==-1)
      {
        printf("Error: File not found.\n");
        continue;
      }

    }
    else if(strcmp(token[0],"read")==0)
    {
    int bytes,offset;
    {
      if(count==0)
      {
        printf("Error: File system image must be opened first.\n");
        continue;
      }
      else if(token[1]=='\0')
      {
        printf("Error: File not found.\n");
        continue;
      }
      else if(token[2]=='\0'|| token[3]=='\0')
      {
        printf("Error: Required values are missing.\n");
        continue;
      }
      uint16_t cluster;
      offset=atoi(token[2]);
      bytes=atoi(token[3]);
      int check_inputFile=file_compare(token[1]);
      if(check_inputFile==-1)
      {
        printf("Error: File not found.\n");
        continue;
      }

      else if(check_inputFile!=-1)
      {
      cluster =dir[check_inputFile].DIR_FirstClusterLow;
      fseek(ptr,LBAToOffset(cluster)+offset,SEEK_SET);
      int bytes_to_read=1;
      while(bytes_to_read<=atoi(token[3]))
      {
        uint8_t value;
        fread(&value,1,1,ptr);
        printf("%x ",value);
        bytes_to_read++;
      }
      printf("\n");

      }
    }
  }
    else if(strcmp(token[0],"ls")==0)
    {
      if(count==0)
      {
        printf("Error: File system image must be opened first.\n");
        continue;
      }
      int i;
      char FileName[12];
      memset(FileName,0,12);

      for(i = 0; i < 16; i++ )
      {
        if( dir[i].Dir_Attr == 0x01||dir[i].Dir_Attr == 0x10||dir[i].Dir_Attr == 0x20 )
        {
          strncpy(FileName,dir[i].DIR_Name,11);
          printf("%s\n",FileName);
        }
      }
  }
  else if(strcmp(token[0],"cd")==0)
  {
    int i;
    uint16_t cluster;
    if(count==0)
    {
      printf("Error: File system image must be opened first.\n");
      continue;
    }
    int check_inputFile=file_compare(token[1]);
    if(check_inputFile==-1)
    {
      printf("Error: File not found.\n");
      continue;
    }
    else
    {
      if(dir[check_inputFile].Dir_Attr == 0x10)
      {
        cluster =dir[check_inputFile].DIR_FirstClusterLow;
        fseek(ptr,LBAToOffset(cluster),SEEK_SET);
        fread(&dir[0],16,sizeof(struct DirectoryEntry),ptr);
      }
    }
  }
  else if(strcmp(token[0],"get")==0)
  {
    char *getfile;
    int i;
    char readable_name[13];
    memset(readable_name,0,13);
    uint16_t cluster;
    if(count==0)
    {
      printf("Error: File system not open.\n");
      continue;
    }
    else if(token[1]=='\0')
    {
      printf("Error: File not found.\n");
      continue;
    }
    strncpy(readable_name,token[1],strlen(token[1]));
    int check_inputFile=file_compare(token[1]);

    if(check_inputFile==-1)
    {
      printf("Error: File not found.\n");
      continue;
    }
    else
    {
      printf("check:%s\n",readable_name);
      for( i = 0; i < 12; i++ )
      {
        readable_name[i] = tolower(readable_name[i]);
      }
      cluster =dir[check_inputFile].DIR_FirstClusterLow;
      FILE *ptr2 =fopen(readable_name,"w+");
      //fseek(ptr,LBAToOffset(cluster),SEEK_SET);
      fseek(ptr,LBAToOffset(cluster),SEEK_SET);
      fread(&getfile,sizeof(getfile),1,ptr);
      fwrite(&getfile,sizeof(getfile),1,ptr2);

      fclose(ptr2);
    }
  }

  else if(strcmp(token[0],"close")==0)
    {
      if(count==0)
      {
        printf("Error: File system not open.\n");
        continue;
      }
      memset(filename,0, sizeof(filename));
      count=0;
      continue;
    }

    free( working_root );
  }
  return 0;
}
