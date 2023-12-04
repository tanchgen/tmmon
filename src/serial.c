/*
 * serial.c
 *
 *  Created on: 30 нояб. 2023 г.
 *      Author: jet Tanchin Gennady <g.tanchin@yandex.ru>
 */

#include <stdio.h>
#include <fcntl.h>   /* File Control Definitions           */
#include <termios.h> /* POSIX Terminal Control Definitions */
#include <unistd.h>  /* UNIX Standard Definitions      */
#include <errno.h>   /* ERROR Number Definitions           */
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "tm_read.h"

extern sGlobalArgs globalArgs;
// ---------------- Input & Output files
const char * fileSuffix = ".csv";
struct timeval tv;
int serfd;/*Serial device file Descriptor*/
int outfd;/* Out file descriptor*/
char fullFname[256];



int serialInit( char * spname ){
  struct termios SerialPortSettings;  /* Create the structure */

  /*------------------------------- Opening the Serial Port -------------------------------*/

  /* Change /dev/ttyUSB0 to the one corresponding to your system */

  serfd = open( spname, O_RDWR | O_NOCTTY);  /* ttyACM is the STM CDC device */
          /* O_RDWR   - Read/Write access to serial port       */
        /* O_NOCTTY - No terminal will control the process   */
        /* Open in blocking mode,read will wait              */



      //if(fd == -1)            /* Error Checking */
   if(serfd < 0) {           /* Error Checking */
     printf("\n  Error! in Opening serial device  ");
     sleep(1);
     return serfd;
   }
   printf("\n  %s Opened Successfully ", spname );

  /*---------- Setting the Attributes of the serial port using termios structure --------- */

  tcgetattr(serfd, &SerialPortSettings); /* Get the current attributes of the Serial port */

  /* Setting the Baud rate */
  cfsetispeed( &SerialPortSettings, B460800 ); /* Set Read  Speed as 460800                       */
  cfsetospeed( &SerialPortSettings, B460800 ); /* Set Write Speed as 460800                       */

  /* 8N1 Mode */
  SerialPortSettings.c_cflag &= ~PARENB;   /* Disables the Parity Enable bit(PARENB),So No Parity   */
  SerialPortSettings.c_cflag &= ~CSTOPB;   /* CSTOPB = 2 Stop bits,here it is cleared so 1 Stop bit */
  SerialPortSettings.c_cflag &= ~CSIZE;  /* Clears the mask for setting the data size             */
  SerialPortSettings.c_cflag |=  CS8;      /* Set the data bits = 8                                 */

  SerialPortSettings.c_cflag &= ~CRTSCTS;       /* No Hardware flow Control                         */
  SerialPortSettings.c_cflag |= CREAD | CLOCAL; /* Enable receiver,Ignore Modem Control lines       */


  SerialPortSettings.c_iflag &= ~(IXON | IXOFF | IXANY);          /* Disable XON/XOFF flow control both i/p and o/p */
  SerialPortSettings.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);  /* Non Cannonical mode                            */

  SerialPortSettings.c_oflag &= ~OPOST;/*No Output Processing*/

  /* Setting Time outs */
  SerialPortSettings.c_cc[VMIN] = 40; /* Read at least 10 characters */
  SerialPortSettings.c_cc[VTIME] = 10; /* Wait indefinetly   */


  if((tcsetattr(serfd,TCSANOW,&SerialPortSettings)) != 0) { /* Set the attributes to the termios structure*/
    printf("\n  ERROR ! in Setting attributes");
  }
  else {
    printf("\n  BaudRate = 460800 \n  StopBits = 1 \n  Parity   = none\n");
  }

  return 0;
}


// Чтение устройства в буфер до символа ch
int serialRead( char * buf, char ch ){
  int bts;
  int size = 0;    /* Number of bytes read by the read() system call */

  while(1) {
    if( (bts = read( serfd, buf, 1 )) < 0 ){
      // Файл устройства недоступен
      perror("Serial device reading fault");
      return bts;
    }
    else {
      size++;
      if( *buf == ch ){
        buf++;
        *buf = '\0';      // Ограничим полученную строку
        break;
      }
      buf++;
    }
  }

  return size;
}


// Чтение данных из последовательного порта
int spRead( void ){
  /*------------------------------- Read data from serial port -----------------------------*/

  char rxBuffer[256];   /* Buffer to store the data received              */
  char * pRxBuf = rxBuffer;
  int size;
  int slen;

  while (size >= 0) {
    struct tm * stm;
    time_t tme;
//    tcflush(fd, TCIFLUSH);   /* Discards old data in the rx buffer  */

    // Ждем начала строки
    if( (size = serialRead( pRxBuf, '"' )) < 0 ){
      break;
    }
    pRxBuf++;
    slen = 1;
    // Открываем файл для записи
    strcpy( fullFname, globalArgs.outDirName );
    strcat( fullFname, globalArgs.filePrefix );
    tme = time( NULL );
    stm = gmtime( &tme );
    strftime( (fullFname + strlen(fullFname)), 18, "%y.%m.%d-%H:%M:%S", stm );
    strcat( fullFname, fileSuffix );

    outfd = open( fullFname, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );
            /* O_RDWR   - Read/Write access to serial port       */
          /* Open in blocking mode,read will wait              */

    if( outfd < 0 ) {           /* Error Checking */
      printf("\n  Error! in Opening output file");
      return serfd;
    }
    printf("\n  %s Opened Successfully ", fullFname );

    // Читаем построчно, пока не прочитаем пустую строку
    do {
      if( (size = serialRead( pRxBuf, '\n' )) < 0 ){
        break;
      }
      slen += size;
      if( write( outfd, rxBuffer, slen ) != slen ){
        perror("Output file writing fault");
        exit( -1 );
      }
      slen = 0;
      pRxBuf = rxBuffer;
    } while( size > 1 );
    if( size < 0 ){
      // Ошибка чтения последовательного порта
      break;
    }
    else {
      // Закончили писать пакет
      close( outfd );
      puts(" Close out file ");
    }
  }

  return size;
}

