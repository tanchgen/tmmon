/*
 * tm_read.h
 *
 *  Created on: 29 нояб. 2023 г.
 *      Author: jet Tanchin Gennady <g.tanchin@yandex.ru>
 */

#ifndef TM_READ_H_
#define TM_READ_H_

enum {
  RESET,
  SET
};

typedef struct _globalArgs {
  char *devName;     // in file name (-i)
  char *outDirName;     // out file directory name (-o)
  char *filePrefix;     // out file directory name (-o)
  const char **param;
} sGlobalArgs;

#endif /* TM_READ_H_ */
