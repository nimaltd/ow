
/*
 * @file        ow.h
 * @brief       OneWire communication driver
 * @author      Nima Askari
 * @version     1.0.0
 * @license     See the LICENSE file in the root folder.
 *
 * @note        All my libraries are dual-licensed. 
 *              Please review the licensing terms before using them.
 *              For any inquiries, feel free to contact me.
 *
 * @github      https://www.github.com/nimaltd
 * @linkedin    https://www.linkedin.com/in/nimaltd
 * @youtube     https://www.youtube.com/@nimaltd
 * @instagram   https://instagram.com/github.nimaltd
 *
 * Copyright (C) 2025 Nima Askari - NimaLTD. All rights reserved.
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
/** Typedef/Struct/Enum **/
/*************************************************************************************************/

/*************************************************************************************************/
/* Error values returned by OneWire operations */
typedef enum
{
  OW_ERR_NONE               = 0,   /* No error */
  OW_ERR_BUSY,                     /* Bus is busy */
  OW_ERR_BUS,                      /* General bus error */
  OW_ERR_RESET,                    /* Reset failed */
  OW_ERR_LEN,                      /* Invalid data length */
  OW_ERR_ROM_ID,                   /* ROM ID error */

} ow_err_t;

/*************************************************************************************************/
/* Internal state machine for OneWire transactions */
typedef enum
{
  OW_STATE_IDLE             = 0,   /* No activity */
  OW_STATE_XFER,                   /* Data transfer ongoing */
  OW_STATE_SEARCH,                 /* Searching devices on bus */
  OW_STATE_DONE,                   /* Operation completed */

} ow_state_t;

/*************************************************************************************************/
/* Encoded values read/written on the OneWire bus */
typedef enum
{
  OW_VAL_DIFF               = 0x00,  /* Both 0 and 1 possible (conflict) */
  OW_VAL_0                  = 0x01,  /* Logic 0 */
  OW_VAL_1                  = 0x10,  /* Logic 1 */
  OW_VAL_ERR                = 0x11,  /* Invalid / error */

} ow_val_t;

/*************************************************************************************************/
/* Standard ROM-level commands defined by OneWire protocol */
typedef enum
{
  OW_CMD_READ_ROM           = 0x33,
  OW_CMD_MATCH_ROM          = 0x55,
  OW_CMD_SKIP_ROM           = 0xCC,
  OW_CMD_SEARCH_ROM         = 0xF0,
  OW_CMD_SEARCH_ALARM       = 0xEC,

} ow_cmd_t;

/*************************************************************************************************/
/* Union representing 64-bit ROM ID (family, serial, crc) */
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

/*************************************************************************************************/
/* Callback prototype used when operation completes */
typedef void (*ow_done_cb_t)(ow_err_t error);

/*************************************************************************************************/
/* Used to configure OneWire handle at startup */
typedef struct
{
  TIM_HandleTypeDef         *tim_handle;                   /* Timer handle */
  GPIO_TypeDef              *gpio;                         /* GPIO port */
  uint16_t                  pin;                           /* GPIO pin */
  void                      (*tim_cb)(TIM_HandleTypeDef*); /* Timer callback */
  ow_done_cb_t              done_cb;                       /* Done callback */

} ow_init_t;

/*************************************************************************************************/
/* Internal buffer used for read/write operations */
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

/*************************************************************************************************/
/* Maintains state for ROM search algorithm */
typedef struct __PACKED
{
  ow_val_t                  val;
  int8_t                    last_discrepancy;
  int8_t                    last_zero;
  uint8_t                   last_device_flag;
  uint8_t                   rom_id[8];

} ow_search_t;

/*************************************************************************************************/
/* Precomputed pin operations for faster bit manipulation */
typedef struct
{
  TIM_HandleTypeDef         *tim_handle;
  GPIO_TypeDef              *gpio;
  uint32_t                  pin_set;
  uint32_t                  pin_reset;
  uint32_t                  pin_read;
  ow_done_cb_t              done_cb;

} ow_config_t;

/*************************************************************************************************/
/* Main driver handle containing state, config and buffers */
typedef struct
{
  ow_config_t               config;                /* Static configuration */
  ow_buf_t                  buf;                   /* Transfer buffer */
  ow_state_t                state;                 /* Current state */
  ow_err_t                  error;                 /* Last error */
  ow_id_t                   rom_id[OW_MAX_DEVICE]; /* List of ROM IDs */
#if (OW_MAX_DEVICE > 1)
  uint8_t                   rom_id_found;          /* Number of devices found */
  ow_search_t               search;                /* Search state */
#endif

} ow_handle_t;

/*************************************************************************************************/
/** API Functions **/
/*************************************************************************************************/

/* Initialize OneWire driver */
void      ow_init(ow_handle_t *handle, ow_init_t *init);

/* Must be called in timer ISR to handle timing */
void      ow_callback(ow_handle_t *handle);

/* Calculate CRC8 for given data */
uint8_t   ow_crc(const uint8_t *data, uint16_t len);

/* Check if driver is busy */
bool      ow_is_busy(ow_handle_t *handle);

/* Get last error code */
ow_err_t  ow_last_error(ow_handle_t *handle);

/* Update device ROM ID(s) on bus */
ow_err_t  ow_update_rom_id(ow_handle_t *handle);

/* Write command + SKIP ROM */
ow_err_t  ow_write_any(ow_handle_t *handle, uint8_t fn_cmd, const uint8_t *data, uint16_t len);

/* Read command + SKIP ROM */
ow_err_t  ow_read_any(ow_handle_t *handle, uint8_t fn_cmd, uint16_t len);

#if (OW_MAX_DEVICE > 1)
/* Write command + data to specific device by ROM ID index */
ow_err_t  ow_write_by_id(ow_handle_t *handle, uint8_t rom_id, uint8_t fn_cmd, const uint8_t *data, uint16_t len);

/* Read command + response from specific device by ROM ID index */
ow_err_t  ow_read_by_id(ow_handle_t *handle, uint8_t rom_id, uint8_t fn_cmd, uint16_t len);

/* Return number of devices found */
uint8_t   ow_devices(ow_handle_t *handle);
#endif

/* Retrieve last response data */
uint16_t  ow_read_resp(ow_handle_t *handle, uint8_t *data, uint16_t data_size);

/*************************************************************************************************/
/** End of File **/
/*************************************************************************************************/

#ifdef __cplusplus
}
#endif
#endif /* _OW_H_ */
