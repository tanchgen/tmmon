/*
 * param.h
 *
 *  Created on: 1 апр. 2021 г.
 *      Author: jet
 */

#ifndef PARAM_H_
#define PARAM_H_

#include "list.h"

typedef struct {
  struct list_head node;
  char * prmname;
  uint64_t time;
  float value;
} sParam;

typedef struct {
  char prmName[50];
  int prmAddr;
  struct list_head * prmhead;
} sPrmList;


#endif /* PARAM_H_ */
