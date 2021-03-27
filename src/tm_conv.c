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
#include <ctype.h>
#include <getopt.h>
#include <math.h>


enum {
  RESET,
  SET
};

struct timezone moscowtz = {
    .tz_minuteswest = -3 * 60,
    .tz_dsttime = 0
};

// ---------------- Input & Output files
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
} sTmHeader;

int readDump( FILE * file, char * buff, size_t size );
// -----------------------------------------------------

// ------------ Get options ----------------------------
struct globalArgs_t {
  const char *inFileName;     // in file name (-i)
  const char *outDirName;     // out file directory name (-o)
  const char *outFileName;     // out file directory name (-o)
  uint16_t fid;               // Protocol ID (-d)
  const char * group;         // Graphic group name (-g)
  uint64_t startTime;      // start time (-b)
  uint64_t stopTime;       // start time (-e)
  const char **param;
} globalArgs = {
    .inFileName = "/home/jet/work/lin.space/tm_mon/tmp/tm.dump",
    .outDirName = "/home/jet/work/lin.space/tm_mon/tmp/",
    .group = "PRM_GROUP",
    .startTime = 0,
    .stopTime = 0,
};

static const char *optString = "f:d:i:g:b:e:p:h?";

static const struct option longOpts[] = {
  { "in-file-name", required_argument, NULL, 'f' },
  { "out-dir-name", required_argument, NULL, 'd' },
  { "fid", required_argument, NULL, 'i' },
  { "group", required_argument, NULL, 'g' },
  { "begin-time", required_argument, NULL, 'b' },
  { "end-time", required_argument, NULL, 'e' },
  { "parameter", required_argument, NULL, 'p' },
  { "help", no_argument, NULL, 'h' },
  { NULL, no_argument, NULL, 0 }
};
int getOptions( int argc, char ** argv );

void display_usage( int arg );
uint64_t stTimeGet( char * opt );
// -----------------------------------------------------

// ------------ Parameter list --------------------------
struct {
  char prmName[50];
  int prmAddr;
} prmList[100];

int paramListGet( char * prmStr, uint idx );
// -----------------------------------------------------

int main( int argc, char **argv ){
  char filename[256] ={0};
  uint8_t ch = 0;
  FILE * tmfile;
  FILE * stlogfile;
  char * pdata;
  uint32_t rxcount = 0;

  sDumpHeader dumpHead;
//  uint64_t mtime;
  struct timeval tv;

  if( getOptions( argc, argv ) ){
    exit(1);
  }

  // Открываем файл принятых пакетов ТМ
  if( (tmfile = fopen( globalArgs.inFileName, "r" )) == NULL ) {
    printf("He удается открыть файл %s.\n", globalArgs.inFileName);
    exit(1);
  }

  // Открываем файл исходящих данных логов STM‑Monitor
  if( (stlogfile = fopen( globalArgs.outFileName, "a" )) == NULL ) {
    printf("He удается открыть файл %s.\n", globalArgs.outFileName);
    exit(1);
  }

  tmReadConv( );


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


// Получение аргументов для программы
int getOptions( int argc, char ** argv ){
  int opt;
  int longIndex;
  uint prmIndex = 0;



  opt = getopt_long( argc, argv, optString, longOpts, &longIndex );
  while( opt != -1 ) {
    switch( opt ) {
      case 'f':
        globalArgs.inFileName = optarg;   // input file name
        break;

      case 'd':
        globalArgs.outDirName = optarg;   // Output dir name
        break;

      case 'i':
        globalArgs.fid = atoi(optarg);    // Protocol ID (-d)
        break;

      case 'g':
        globalArgs.group = optarg;    // Group name (-g)
        break;

      case 'b':{
        globalArgs.startTime = stTimeGet( optarg );    // start time (-b)
        break;
      }

      case 'e':{
        globalArgs.stopTime = stTimeGet( optarg );    // stop time (-e)
        break;
      }

      case 'p':{
        paramListGet( optarg, prmIndex );
        prmIndex++;
        break;
      }
      case 'h':   /* намеренный проход в следующий case-блок */
        display_usage( 0 );
        break;
      case '?':
        display_usage( -1 );
        break;

      case 0:     /* длинная опция без короткого эквивалента */
//        if( strcmp( "randomize", longOpts[longIndex].name ) == 0 ) {
//          globalArgs.randomized = 1;
//        }
//        else {
//          display_usage();
//          return -1;
//        }
        break;

      default:
          /* сюда попасть невозможно. */
          break;
    }

    opt = getopt_long( argc, argv, optString, longOpts, &longIndex );
  }

  // Коррекция параметров
  if( globalArgs.stopTime == 0 ){
    // Выставляем текущее системное время
    struct timeval tv;

    gettimeofday( &tv, &moscowtz );
    globalArgs.stopTime = tv.tv_sec * 1000 + tv.tv_usec / 1000;

    globalArgs.stopTime &= 0xFFFFFFFFFFFF; // Обрезаем до 6-и байт
  }
  // Время остановки не может быть меньше времени старта
  globalArgs.stopTime = max( globalArgs.startTime, globalArgs.stopTime );

  // Имя исходящего файла
  sprintf( globalArgs.outFileName, "%s\\Log_%X_%s_%f-%f.msct",
            globalArgs.outDirName, globalArgs.fid, globalArgs.group,
            globalArgs.startTime, globalArgs.stopTime);

  return 0;
}


// Время из аргумента программы
uint64_t stTimeGet( char * opt ){
  uint64_t time = -1;
  double tf;

  if( (opt == NULL) || (*opt == '-') ){
    display_usage( -1 );
  }

  // Секунды
  tf = atof( opt );

  if( tf < 0 ){
    time = 0;
  }
  else {
    time = floor( tf * 1000 );
    time &= 0xFFFFFFFFFFFF;
  }

  return time;
}

// Параметр из аргумента программы
int paramListGet( char * prmStr, uint idx ){
  int rc = -1;  // Возвращаемое значение - ошибка

  char * commaPos;

  if( (commaPos = strstr(prmStr, "," )) == NULL ){
    return rc;
  }

  strncpy( prmList[idx].prmName, prmStr, commaPos - prmStr );

  prmStr = commaPos + 1;

  if( (prmList[idx].prmAddr = atoi( prmStr )) == 0 ){
    display_usage( rc );
  }
  else {
    rc = 0;
  }

  return rc;
}


void tmReadConv( void ){
  uint64_t imtime;        // Время промежуточного финала записи параметров
  uint64_t mtime;

  uint64_t imtime;        // Время промежуточного финала записи параметров

  // Читаем файл ТМ
  for( imtime = globalArgs.startTime + 50;; ){
    // Читаем время и размер
    if( fread( tmHead, sizeof(sTmHeader), 1, tmfile ) != sizeof(sTmHeader) ){
      // Достигнут конец файла - выходим
      break;
    }

    mtime = (tmHead.btime << 32) || (tmHead.mtime << 16) || (tmHead.ltime);
    if( mtime > globalArgs.stopTime ){
      // Время пакета (и всех последующих) больше время конца - выходим
      break;
    }

    if( (tmHead.fid != globalArgs.fid)
        || (mtime < globalArgs.startTime)
        ){
      // Этот пакет не тот, что нужен или время пакета меньше стартового - пропускаем его


    }

  }

  // Прочитан весь файл / до конечного времени


}

/*
{ "in-file-name", required_argument, NULL, 'f' },
{ "out-dir-name", required_argument, NULL, 'd' },
{ "fid", required_argument, NULL, 'i' },
{ "begin-time", required_argument, NULL, 'b' },
{ "end-time", required_argument, NULL, 'e' },
{ "parameter", required_argument, NULL, 'p' },
{ "help", no_argument, NULL, 'h' },
*/

// Вывод информации об использовании
void display_usage( int arg ){
  puts(" Usage tm_conv: tm_conv -[fdibeph]");
  puts("\t-f, --in-file-name=<file>    input file name");
  puts("\t-d, --out-dir-name=<dir>     input directory name");
  puts("\t-i, --fid=<fid>              project ID");
  puts("\t-b, --begin-time=<time>      start time in format: <UnixTime>.<msec>");
  puts("\t-e, --end-time=<time>        stop time in format: <UnixTime>.<msec>");
  puts("\t-p, --parameter=<parameter_name>,<parameter_address>      parameter name and parameter address in TM packet:");
  puts("\t                                                          <parameter address> >= 10");
  puts("\t-h, --help      this display");

  exit(arg);
}
