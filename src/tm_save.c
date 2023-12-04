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

#include "tm_save.h"

// -----------------------------------------------------
extern int serfd;/*Serial device file Descriptor*/
extern char serDn[256];

// ------------ Get options ----------------------------
sGlobalArgs globalArgs = {
    .devName = "/dev/ttyACM",
    .outDirName = "../tmp/",
    .filePrefix = "alcodata.",
};

static const char *optString = "D:d:p:h?";

static const struct option longOpts[] = {
  { "device", required_argument, NULL, 'D' },
  { "out-dir-name", required_argument, NULL, 'd' },
  { "prefix", required_argument, NULL, 'p' },
  { "help", no_argument, NULL, 'h' },
  { NULL, no_argument, NULL, 0 }
};
int getOptions( int argc, char ** argv );

void display_usage( int arg );
// -----------------------------------------------------
int serialInit( char * spname );
int spRead( void );

// -----------------------------------------------------

int main( int argc, char **argv ){

  if( getOptions( argc, argv ) ){
    exit(1);
  }

  while( 1 ){
	int rc;

    // Ждем, пока не появится устройство tty*
    printf(" Wait for the serial device %s to show up.\n", globalArgs.devName );
    while( serialInit( globalArgs.devName ) )
    {}
    // Устройство появилось
    printf("Serial device %s has been found.\n", serDn );

    if( (rc = spRead()) < 0 ){
      if(rc == -2 ){
    	return -1;
      }
      else {
		// Устройство пропало
    	close( serfd );
		printf(" Serial device %s is lost.\n", globalArgs.devName );
		continue;
      }
    }
  }

  return 1;
}



// Получение аргументов для программы
int getOptions( int argc, char ** argv ){
  int opt;
  int longIndex;

  opt = getopt_long( argc, argv, optString, longOpts, &longIndex );
  while( opt != -1 ) {
    switch( opt ) {
      case 'D':
        globalArgs.devName = optarg;   // input file name
        break;

      case 'd':
        globalArgs.outDirName = optarg;   // Output dir name
        break;

      case 'p':{
        globalArgs.filePrefix = optarg;   // Output file name prefix
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

  return 0;
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
  puts("\t-D, --device=<file>          tty-device file name");
  puts("\t-d, --out-dir-name=<dir>     output directory name");
  puts("\t-p, --prefix=<prefix_name>   prefix filename");
  puts("\t-h, --help      this display");

  exit(arg);
}
