
/*
 * @file        ONEW
 * @author      Nima Askari
 * @version     1.0.0
 * @license     See the LICENSE file in the root folder.
 *
 * @github      https://www.github.com/nimaltd
 * @linkedin    https://www.linkedin.com/in/nimaltd
 * @youtube     https://www.youtube.com/@nimaltd
 * @instagram   https://instagram.com/github.nimaltd
 *
 * Copyright (C) 2025 Nima Askari - NimaLTD. All rights reserved.
 *
 */

/*
 *  ONEW (One Wire) Library Help
 *
 *  Overview:

 */

#ifndef _OW_H_
#define _OW_H_

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************************************************************/
/** Includes **/
/*************************************************************************************************/

#include <stdbool.h>
#include "main.h"
#include "tim.h"
#include "ow_config.h"

/*************************************************************************************************/
/** Strucs and Enums **/
/*************************************************************************************************/

typedef enum
{
  OW_ERR_NONE               = 0,
  OW_ERR_BUSY,
  OW_ERR_BUS,
  OW_ERR_RESET,
  OW_ERR_LEN,
  OW_ERR_ROM_ID,

} ow_err_t;

typedef enum
{
  OW_STATE_IDLE             = 0,
  OW_STATE_XFER,
  OW_STATE_SEARCH,
  OW_STATE_DONE,

} ow_state_t;

typedef enum
{
  OW_CMD_READ_ROM           = 0x33,
  OW_CMD_MATCH_ROM          = 0x55,
  OW_CMD_SKIP_ROM           = 0xCC,
  OW_CMD_SEARCH_ROM         = 0xF0,
  OW_CMD_SEARCH_ALARM       = 0xEC,

} ow_cmd_t;

typedef union
{
  uint8_t                   rom_id_array[8];
  struct  __PACKED
  {
    uint8_t                 family;
    uint8_t                 serial[6];
    uint8_t                 crc;

  } rom_id_struct;

} ow_id_t;


typedef struct
{
  TIM_HandleTypeDef         *tim_handle;
  GPIO_TypeDef              *gpio;
  uint16_t                  pin;
  void                      (*tim_cb)(TIM_HandleTypeDef*);

} ow_init_t;

typedef struct
{
#if (OW_MAX_DEVICE > 1)
  uint8_t                   data[1 + 8 + 1 + OW_MAX_DATA_LEN];
#else
  uint8_t                   data[1 + 1 + OW_MAX_DATA_LEN];
#endif
  uint8_t                   bit_ph;
  uint8_t                   bit_idx;
  uint16_t                  byte_idx;
  uint16_t                  write_len;
  uint16_t                  read_len;

} ow_buf_t;

typedef struct
{
  TIM_HandleTypeDef         *tim_handle;
  GPIO_TypeDef              *gpio;
  uint32_t                  pin_set;
  uint32_t                  pin_reset;
  uint32_t                  pin_read;

} ow_config_t;

typedef struct
{
  ow_config_t               config;
  ow_buf_t                  buf;
  ow_state_t                state;
  ow_err_t                  error;
#if (OW_MAX_DEVICE > 1)
  uint8_t                   rom_id_found;
  ow_id_t                   rom_id[OW_MAX_DEVICE];
  uint8_t                   search_val;
  uint8_t                   search_found;
  int8_t                    search_diff_idx;
  int16_t                   search_last_discrepancy;
  bool                      search_last_device_flag;
  uint8_t                   search_last_rom[8];
#endif

} ow_handle_t;

/*************************************************************************************************/
/** Function Declarations **/
/*************************************************************************************************/

void      ow_init(ow_handle_t *handle, ow_init_t *init);
void      ow_callback(ow_handle_t *handle);
bool      ow_is_busy(ow_handle_t *handle);
ow_err_t  ow_last_error(ow_handle_t *handle);

ow_err_t  ow_read_single_rom_id(ow_handle_t *handle);
ow_err_t  ow_write(ow_handle_t *handle, uint8_t fn_cmd, const uint8_t *data, uint16_t len);
ow_err_t  ow_read(ow_handle_t *handle, uint8_t fn_cmd, uint16_t len);
#if (OW_MAX_DEVICE > 1)
ow_err_t  ow_update_all_rom_id(ow_handle_t *handle);
ow_err_t  ow_write_by_id(ow_handle_t *handle, uint8_t rom_id, uint8_t fn_cmd, const uint8_t *data, uint16_t len);
ow_err_t  ow_read_by_id(ow_handle_t *handle, uint8_t rom_id, uint8_t fn_cmd, uint16_t len);
#endif

uint16_t  ow_read_resp(ow_handle_t *handle, uint8_t *data, uint16_t data_size);

/*************************************************************************************************/
/** End of File **/
/*************************************************************************************************/

#ifdef __cplusplus
}
#endif
#endif /* _OW_H_ */
