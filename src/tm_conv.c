/*
 ============================================================================
 Name        : tm_conv.c
 Author      : G.Tanchin
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>

enum {
  RESET,
  SET
};

struct timezone moscowtz = {
    .tz_minuteswest = -3 * 60,
    .tz_dsttime = 0
};

const char * dir = "/home/jet/work/lin.space/tm_mon/tmp/";
const char * infile = "tm.dump";
const char * outfile = "tm.out";


typedef struct __attribute__((__packed__)) {
  uint16_t faddr;
  uint16_t fsize;
  uint16_t ftipe;
  uint16_t fid;
  uint16_t ne;
  uint16_t nr;
  uint16_t fver;
} sDumpHeader;

typedef struct __attribute__((__packed__)) {
  uint16_t ltime;     // Младшие байты метки времени
  uint16_t mtime;     // Средние байты метки времени
  uint16_t btime;     // Старшие байты метки времени
  uint16_t fsize;
  uint16_t fid;
  uint16_t ne;
  uint16_t nr;
} sOutHeader;


int readDump( FILE * file, char * buff, size_t size );

int main( void ){
  char filename[256];
  uint8_t ch = 0;
  FILE * dumpfile;
  FILE * tmfile;
  char * pdata;
  uint32_t rxcount = 0;

  sDumpHeader dumpHead;
  sOutHeader outHead;
  uint64_t mtime;
  struct timeval tv;

  strcpy(filename, dir);
  strcat( filename, infile);

  if( (dumpfile = fopen( filename, "r" )) == NULL ) {
    printf("He удается открыть файл %s.\n", filename);
    exit(1);
  }

  strcpy(filename, dir);
  strcat( filename, outfile);

  if( (tmfile = fopen( filename, "a" )) == NULL ) {
    printf("He удается открыть файл %s.\n", filename);
    exit(1);
  }


  // Ищем начало пакета
  do {
    fread( &ch, sizeof(ch), 1, dumpfile );
    if( feof( dumpfile ) ){
      perror("Fail: End Of File - Any packets not found");
      exit(1);
    }
  } while( ch != 0xC0 );

  // Считываем заголовок
  if( readDump( dumpfile, (char*)&dumpHead, sizeof(dumpHead) ) ){
    exit(3);
  }

  gettimeofday( &tv, &moscowtz );
  mtime = tv.tv_sec * 1000 + tv.tv_usec / 1000;

  mtime &= 0xFFFFFFFFFFFF; // Обрезаем до 6-и байт

  // Считываем основной блок данных
  rxcount = dumpHead.fsize - 6 * 2; // Заголовок уже считан, CRC не читаем
  // Память под блок данных
  if( (pdata = malloc( rxcount )) == NULL ){
    perror( "Memory allocation error" );
    exit(2);
  }

  if( readDump( dumpfile, pdata, rxcount ) ){
    exit(3);
  }

  // Формируем заголовок
  outHead.ltime = mtime & 0xFFFF;
  outHead.mtime = (mtime >> 16) & 0xFFFF;
  outHead.btime = (mtime >> 32) & 0xFFFF;
  outHead.fsize = rxcount + 6;
  outHead.fid = dumpHead.fid;
  outHead.ne = dumpHead.ne;
  outHead.nr = dumpHead.nr;

  // Запись в файл ТМ
  // Пишем заголовок
#define BLOCK_COUNT    1
  if( fwrite( &outHead, sizeof(outHead), BLOCK_COUNT, tmfile ) != BLOCK_COUNT){
    perror( "Fault write to TMfile" );
    return -1;
  }
  // Пишем блок данных
  if( fwrite( pdata, rxcount, BLOCK_COUNT, tmfile ) != BLOCK_COUNT ){
    perror( "Fault write to TMfile" );
    return -1;
  }

  free(pdata);

  fclose( dumpfile );
  fclose( tmfile );

  return 0;
}


int readDump( FILE * file, char * buff, size_t size ){
  uint8_t ch;
  uint8_t staffFlag = RESET;

  for( int i = 0; i < size; ){
    fread( &ch, sizeof(ch), 1, file );

    if( feof( file ) ){
      perror( "Unexpected end of file" );
      return -1;
    }

    if( staffFlag == RESET ){
      if( ch == 0xDB ){
        staffFlag = SET;
        continue;
      }
    }
    else {
      // Байт-стаффинг установлен
      if( ch == 0xDC ){
        ch = 0xC0;
      }
      else if( ch == 0xDD ){
        ch = 0xDB;
      }
      else {
        perror( "Input data not valid" );
        return -1;
      }

      staffFlag = RESET;
    }

    buff[i++] = ch;
  }

  return 0;
}
