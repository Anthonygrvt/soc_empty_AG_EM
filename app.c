/***************************************************************************//**
 * @file
 * @brief Core application logic.
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/
#include "em_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "app.h"
#include "app_log.h"
#include "sl_sensor_rht.h"
#include "temperature.h"
#include "gatt_db.h"
#include "sl_bt_api.h"
#include "sl_simple_timer.h"
#include <sl_bt_types.h>
#include <sl_simple_led_instances.h>


// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

sl_simple_timer_t timer;
int tim=0;
int i1;

struct callback_data_s {
  uint8_t connection;
  uint16_t characteristic;
};

struct callback_data_s callback_data;
struct callback_data_s callback_data1;


void callback(sl_simple_timer_t *timer, void *data)

{
  int32_t ble_temp = convert_carte_to_BLE();
  timer = timer;
  struct callback_data_s * p_my_s = data;
  app_log_info("Timer step %i \n",tim);
  tim=tim+1;

  sl_bt_gatt_server_send_notification(p_my_s->connection,
                                      p_my_s->characteristic,
                                      2,
                                      (const uint8_t *)&ble_temp);

};


/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
SL_WEAK void app_init(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////

  app_log_info("%s\n", __FUNCTION__);
}

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
SL_WEAK void app_process_action(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application code here!                              //
  // This is called infinitely.                                              //
  // Do not call blocking functions from here!                               //
  /////////////////////////////////////////////////////////////////////////////
}

/**************************************************************************//**
 * Bluetooth stack event handler.
 * This overrides the dummy weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  sl_status_t sc;

  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:
      // Create an advertising set.
      sc = sl_bt_advertiser_create_set(&advertising_set_handle);
      app_assert_status(sc);

      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      // Set advertising interval to 100ms.
      sc = sl_bt_advertiser_set_timing(
        advertising_set_handle,
        160, // min. adv. interval (milliseconds * 1.6)
        160, // max. adv. interval (milliseconds * 1.6)
        0,   // adv. duration
        0);  // max. num. adv. events
      app_assert_status(sc);
      // Start advertising and enable connections.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      app_log_info("%s: connection_opened!\n", __FUNCTION__);

      sc = sl_sensor_rht_init();

      sl_simple_led_init_instances();

      app_log_info("status flags init : %i \n",evt->data.evt_gatt_server_characteristic_status.status_flags);

      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      app_log_info("%s: connection_closed!\n", __FUNCTION__);
      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      // Restart advertising after client has disconnected.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);

      //sc = sl_sensor_rht_deinit();

      break;

    case sl_bt_evt_gatt_server_user_read_request_id:
      app_log_info("%s: acces en lecture!\n", __FUNCTION__);

      if(evt->data.evt_gatt_server_user_read_request.characteristic==gattdb_temperature){

         app_log_info("demande de lecture de la temperature \n");

         int32_t ble_temp = convert_carte_to_BLE(); //

         app_log_info("valeur de la temperature convertie en ble : %ld ", ble_temp);

         uint16_t sent_len;

         sc = sl_bt_gatt_server_send_user_read_response(evt->data.evt_gatt_server_user_read_request.connection,
                                                        evt->data.evt_gatt_server_user_read_request.characteristic,
                                                        0,
                                                        32,
                                                        (const uint8_t*)&ble_temp,
                                                        &sent_len);

      }

      break;

    case sl_bt_evt_gatt_server_characteristic_status_id:

      if(evt->data.evt_gatt_server_characteristic_status.status_flags == 0x01)
        {


          app_log_info("status flags : %i \n",evt->data.evt_gatt_server_characteristic_status.status_flags);

          if(evt->data.evt_gatt_server_characteristic_status.characteristic==gattdb_temperature)
            {

                app_log_info("status flags : %i \n",evt->data.evt_gatt_server_characteristic_status.status_flags);

                app_log_info("appui sur bouton Notify \n");


                if(evt->data.evt_gatt_server_characteristic_status.client_config_flags == 0x01)
                  {

                    app_log_info("client_config_flags : %i \n",evt->data.evt_gatt_server_characteristic_status.client_config_flags);

                    callback_data.connection = evt->data.evt_gatt_server_characteristic_status.connection;
                    callback_data.characteristic = evt->data.evt_gatt_server_characteristic_status.characteristic;


                   sl_simple_timer_start(&timer,
                                         1000,
                                         callback,
                                         &callback_data,
                                         true);

                  }

                if(evt->data.evt_gatt_server_characteristic_status.client_config_flags == 0x00)
                  {
                   sl_simple_timer_stop(&timer);

                  }

            }

        }

      break;

    case sl_bt_evt_gatt_server_user_write_request_id:

      app_log_info("Write sur la caractéristique \n");

      uint8array *pvalue = &evt->data.evt_gatt_server_user_write_request.value;
      uint8_t * data = pvalue->data;

      /*for (i1=0 ; i1<pvalue->len; i1++)
        {

          app_log_info("%d",pvalue->data[i1]);

        }*/

      uint8_t opcode = evt->data.evt_gatt_server_user_write_request.att_opcode;
      app_log_info("opcode = %d",opcode);

      if(opcode == 18)
        {
          callback_data1.connection = evt->data.evt_gatt_server_user_write_request.connection;
          callback_data1.characteristic = evt->data.evt_gatt_server_user_write_request.characteristic;
          sl_bt_gatt_server_send_write_response(callback_data1.connection,callback_data1.characteristic,0);
        }
      else{}


      if(data[0]==0)
         {
          sl_led_led0.turn_off(sl_led_led0.context);
         }

      if(data[0]==1)
        {
          sl_led_led0.turn_on(sl_led_led0.context);
        }

      break;




    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////

    // -------------------------------
    // Default event handler.
    default:
      break;
  }

}
