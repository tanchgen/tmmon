/*
 * param.c
 *
 *  Created on: 1 апр. 2021 г.
 *      Author: jet
 */

#include "param.h"

int prmInsert( uint16_t val, ,uint64_t mtime, uint8_t idx){
  sParam * prm;
  struct list_head * pnode;

  if( (prm = malloc( sizeof(sParam))) == NULL ){
    // Ошибка выделения памяти
    return -1;
  }

  pnode = (struct list_head *)prm;
  // Добавим очередное значение параметра в хвост списка
  if( prmList[idx].prmhead == NULL ){
    // Записываем на месото головы
    prmList[idx].prmhead = pnode;
    INIT_LIST_HEAD( prmList[idx].prmhead );
  }
  else {
    // Вставляем в хвост
    list_add_tail( pnode, prmList[idx].prmhead );
  }

  prm->prmname = prmList[idx].prmName;
  prm->time = mtime;
  prm->value = pmbus_data_linear_11_to_regular(val);

  return 0;
}
