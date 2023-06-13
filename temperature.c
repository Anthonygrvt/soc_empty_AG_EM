/*
 * temperature.c
 *
 *  Created on: 30 mai 2023
 *      Author: anthonygravat
 */

#include "sl_sensor_rht.h"

int32_t convert_carte_to_BLE(){

  int32_t t;
  uint32_t rh ;
  sl_sensor_rht_get(&rh , &t);

  int32_t temperature_ble = t / 10;

  return temperature_ble;
}


