#include <stdbool.h>
#include <math.h>

#include "pmbus.h"

float pmbus_data_linear_11_to_regular(uint16_t data)
{
	int16_t  exponent, mantissa;

	/* Simple case. */
	if (data == 0)
		return 0.0f;

	/* LINEAR-11 */
	exponent = (data >> 11) & 0x001f;
	mantissa = data & 0x07ff;

	/* Sign extend exponent. */
	if (exponent > 0x0f)
		exponent |= 0xffe0;

	/* Sign extend mantissa. */
	if (mantissa > 0x03ff)
		mantissa |= 0xf800;

	if (exponent >= 0)
		return ((float)mantissa * (1 << exponent));

	return ((float)mantissa / (1 << -exponent));
}

uint16_t pmbus_data_regular_to_linear_11(float data)
{
	bool     is_negative;
	int16_t  exponent, mantissa;

	/* Simple cases. */
	if (!islessgreater(data, 0.0f))
		return 0;

	is_negative = signbit(data);

	if (isinf(data))
		return (is_negative ? 0x7c00 : 0x7bff);

	data = fabs(data);

	exponent = 0;

	/* Reduce large mantissa until it fits into 10 bit. */
	while (data >= 1023.0f && exponent < 15) {
		++exponent;
		data /= 2.0f;
	}
	/* Increase small mantissa to improve precision. */
	while (data < 511.0f && exponent > -15) {
		--exponent;
		data *= 2.0f;
	}

	mantissa = (int16_t)rintf(data);

	/* Ensure that resulting number is within range. */
	if (mantissa > 0x3ff)
		mantissa = 0x3ff;

	/* Restore sign. */
	if (is_negative)
		mantissa = -mantissa;

	/* Convert to 5 bit exponent, 11 bit mantissa. */
	return (mantissa & 0x7ff) | ((exponent << 11) & 0xf800);
}

/**
  * @brief  Преобразование данных в Linear_11
  *
  * @param[in] data  Преобразовываемые данные (ед.изм. = 1/k от основной ед.)
  *
  * @retval Данные в формате linear_11
  */
uint16_t toLinear11( int32_t data, uint16_t k ){
  bool     is_negative = false;
  int16_t  exponent = 0;

  /* Simple cases. */
  if( data == 0 ){
    return 0;
  }

  if( k == (uint16_t)~(0) ){
    return 0x7bff;
  }

  if( data < 0){
    is_negative = true;
    data = -data;
  }

  data <<= 12;
  data /= k;

  /* Reduce large mantissa until it fits into 10 bit. */
  while (data >= (1023 << 12) && exponent < 15) {
    ++exponent;
    // Округляем при делении
    data++;
    data /= 2;
  }
  /* Increase small mantissa to improve precision. */
  while (data < (511 << 12) && exponent > -15) {
    --exponent;
    data *= 2;
  }

  data >>= 12;
  data &= 0x3FF;

  /* Restore sign. */
  if (is_negative){
    data |= 0x400;
  }

  /* Convert to 5 bit exponent, 11 bit mantissa. */
  return ( data | ((exponent << 11) & 0xf800) );
}

/**
  * @brief  Преобразование данных из Linear_11
  *
  * @param[in] data  Данные в формате linear_11
  *
  * @retval Преобразованные данные (ед.изм. = 1/4096 от основной ед.)
  */
int32_t fromLinear11( int32_t data, uint16_t k  ){
  bool     is_negative = false;
  int16_t  exponent;

  int32_t  mantissa;

  /* Simple case. */
  if (data == 0)
    return 0;

  /* LINEAR-11 */
  exponent = (data >> 11) & 0x001f;
  mantissa = data & 0x07ff;

  /* Sign extend mantissa. */
  if (mantissa > 0x03ff){
    mantissa |= 0xf800;
  }
  mantissa *= k;
  /* Sign extend exponent. */
  if (exponent > 0x0f){
    exponent |= 0xffe0;
    return (mantissa / (1 << -exponent));
  }
  else {
   return (mantissa * (1 << exponent));
  }


  /* Simple cases. */
  if( data == 0 ){
    return 0;
  }

  if( data < 0){
    is_negative = true;
    data = -data;
  }

  /* Reduce large mantissa until it fits into 10 bit. */
  while (data >= (1023 * 4096) && exponent < 15) {
    ++exponent;
    // Округляем при делении
    data++;
    data /= 2;
  }
  /* Increase small mantissa to improve precision. */
  while (data < (511 * 4096) && exponent > -15) {
    --exponent;
    data *= 2;
  }

  data >>= 12;
  data &= 0x3FF;

  /* Restore sign. */
  if (is_negative){
    data |= 0x400;
  }

  /* Convert to 5 bit exponent, 11 bit mantissa. */
  return ( data | ((exponent << 11) & 0xf800) );
}

