
/*
 * @file        OW
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

/*************************************************************************************************/
/** Includes **/
/*************************************************************************************************/

#include <string.h>
#include "ow.h"
#include "tim.h"

/*************************************************************************************************/
/** private variables **/
/*************************************************************************************************/


/*************************************************************************************************/
/** Private Function prototype **/
/*************************************************************************************************/

ow_err_t  ow_start(ow_handle_t *handle);
void      ow_stop(ow_handle_t *handle);
uint8_t   ow_crc(const uint8_t *data, uint16_t len);

__STATIC_FORCEINLINE void ow_state_rst(ow_handle_t *handle);
__STATIC_FORCEINLINE void ow_state_rom_cmd(ow_handle_t *handle);
__STATIC_FORCEINLINE void ow_state_rom_id(ow_handle_t *handle);
__STATIC_FORCEINLINE void ow_state_read_rom_id(ow_handle_t *handle);
__STATIC_FORCEINLINE void ow_state_fn_cmd(ow_handle_t *handle);
__STATIC_FORCEINLINE void ow_state_update_rom(ow_handle_t *handle);
__STATIC_FORCEINLINE void ow_state_update_alarm(ow_handle_t *handle);
__STATIC_FORCEINLINE void ow_state_data_w(ow_handle_t *handle);
__STATIC_FORCEINLINE void ow_state_data_r(ow_handle_t *handle);

__STATIC_FORCEINLINE void ow_write_bit(ow_handle_t *handle, bool high);
__STATIC_FORCEINLINE uint8_t ow_read_bit(ow_handle_t *handle);

/*************************************************************************************************/
/** Function Implementations **/
/*************************************************************************************************/

/*************************************************************************************************/
/**
 */
void ow_init(ow_handle_t *handle, ow_init_t *init)
{
  assert_param(handle != NULL);
  assert_param(init != NULL);
  assert_param(init->gpio != NULL);
  assert_param(init->tim_handle != NULL);
  assert_param(init->tim_cb != NULL);
  handle->gpio = init->gpio;
  handle->pin_set = init->pin;
  handle->pin_reset = init->pin << 16UL;
  handle->pin_read = init->pin;
  handle->tim_handle = init->tim_handle;
  HAL_TIM_RegisterCallback(handle->tim_handle, HAL_TIM_PERIOD_ELAPSED_CB_ID, init->tim_cb);
  ow_write_bit(handle, true);
}

/*************************************************************************************************/
/**
 */
void ow_callback(ow_handle_t *handle)
{
  assert_param(handle != NULL);
  switch (handle->state)
  {
  case OW_STATE_RST:
    ow_state_rst(handle);
    break;
  case OW_STATE_ROM_CMD:
    ow_state_rom_cmd(handle);
    break;
  case OW_STATE_ROM_ID:
    ow_state_rom_id(handle);
    break;
  case OW_STATE_READ_ROM_ID:
    ow_state_read_rom_id(handle);
    break;
  case OW_STATE_FN_CMD:
    ow_state_fn_cmd(handle);
    break;
  case OW_STATE_UPDATE_ROM_ID:
    ow_state_update_rom(handle);
    break;
  case OW_STATE_UPDATE_ALARM_ID:
    ow_state_update_alarm(handle);
    break;
  case OW_STATE_DATA_W:
    ow_state_data_w(handle);
    break;
  case OW_STATE_DATA_R:
    ow_state_data_r(handle);
    break;
  default:
    ow_stop(handle);
    break;
  }
}

/*************************************************************************************************/
/**
 */
bool ow_is_busy(ow_handle_t *handle)
{
  assert_param(handle != NULL);
  return (handle->state != OW_STATE_IDLE);
}

/*************************************************************************************************/
/**
 */
ow_err_t ow_last_error(ow_handle_t *handle)
{
  return handle->error;
}

/*************************************************************************************************/
/**
 */
ow_err_t ow_update_all_id(ow_handle_t *handle)
{
  assert_param(handle != NULL);
  do
  {
    handle->error = ow_start(handle);
    if (handle->error != OW_ERR_NONE)
    {
      ow_stop(handle);
      break;
    }
    handle->rom_max = 0;
    memset(handle->rom_id, 0, sizeof(handle->rom_id));
    handle->buf.rom_cmd = OW_CMD_SEARCH_ROM;

  } while (0);

  return handle->error;
}

#if (OW_USE_ALARM == 1)
/*************************************************************************************************/
/**
 */
ow_err_t ow_update_alarm_id(ow_handle_t *handle)
{
  assert_param(handle != NULL);
  do
  {
    handle->error = ow_start(handle);
    if (handle->error != OW_ERR_NONE)
    {
      ow_stop(handle);
      break;
    }
    memset(handle->alarm_id, 0, sizeof(handle->alarm_id));
    memset(handle->alarm_idx, 0, sizeof(handle->alarm_idx));
    handle->buf.rom_cmd = OW_CMD_SEARCH_ALARM;

  } while (0);

  return handle->error;
}
#endif

/*************************************************************************************************/
/**
 */
ow_err_t ow_read_id(ow_handle_t *handle)
{
  assert_param(handle != NULL);
  do
  {
    handle->error = ow_start(handle);
    if (handle->error != OW_ERR_NONE)
    {
      ow_stop(handle);
      break;
    }
    memset(handle->rom_id, 0, sizeof(handle->rom_id));
    handle->buf.rom_cmd = OW_CMD_READ_ROM;

  } while (0);

  return handle->error;
}

/*************************************************************************************************/
/**
 */
ow_err_t ow_write(ow_handle_t *handle, uint8_t fn_cmd, const uint8_t *data, uint16_t len)
{
  assert_param(handle != NULL);
  do
  {
    handle->error = ow_start(handle);
    if (handle->error != OW_ERR_NONE)
    {
      ow_stop(handle);
      break;
    }
    if (len > OW_MAX_DATA_LEN)
    {
      handle->error = OW_ERR_PARAM;
      ow_stop(handle);
      break;
    }
    if ((data != NULL) && (len > 0))
    {
      memcpy(handle->buf.data, data, len);
    }
    if ((data == NULL) && (len > 0))
    {
      handle->error = OW_ERR_PARAM;
      ow_stop(handle);
      break;
    }
    handle->buf.w_len = len;
    handle->buf.rom_cmd = OW_CMD_SKIP_ROM;
    handle->buf.fn_cmd= fn_cmd;

  } while (0);

  return handle->error;
}

/*************************************************************************************************/
/**
 */
ow_err_t ow_read(ow_handle_t *handle, uint8_t fn_cmd, uint16_t len)
{
  assert_param(handle != NULL);
  do
  {
    handle->error = ow_start(handle);
    if (handle->error != OW_ERR_NONE)
    {
      ow_stop(handle);
      break;
    }
    if (len > OW_MAX_DATA_LEN)
    {
      handle->error = OW_ERR_PARAM;
      ow_stop(handle);
      break;
    }
    handle->buf.r_len = len;
    handle->buf.rom_cmd = OW_CMD_SKIP_ROM;
    handle->buf.fn_cmd= fn_cmd;

  } while (0);

  return handle->error;
}

#if (OW_MAX_DEVICE > 1)
/*************************************************************************************************/
/**
 */
ow_err_t ow_write_by_id(ow_handle_t *handle, uint8_t rom_id, uint8_t fn_cmd, const uint8_t *data, uint16_t len)
{
  assert_param(handle != NULL);
  do
  {
    handle->error = ow_start(handle);
    if (handle->error != OW_ERR_NONE)
    {
      ow_stop(handle);
      break;
    }
    if (len > OW_MAX_DATA_LEN)
    {
      handle->error = OW_ERR_PARAM;
      ow_stop(handle);
      break;
    }
    if (rom_id > handle->rom_max)
    {
      handle->error = OW_ERR_MISS_ROM;
      ow_stop(handle);
      break;
    }
    if ((data == NULL) && (len > 0))
    {
      handle->error = OW_ERR_PARAM;
      ow_stop(handle);
      break;
    }
    if ((data != NULL) && (len > 0))
    {
      memcpy(handle->buf.data, data, len);
    }
    handle->buf.rom_id = rom_id;
    handle->buf.w_len = len;
    handle->buf.rom_cmd = OW_CMD_SKIP_ROM;
    handle->buf.fn_cmd= fn_cmd;

  } while (0);

  return handle->error;
}
#endif

#if (OW_MAX_DEVICE > 1)
/*************************************************************************************************/
/**
 */
ow_err_t ow_read_by_id(ow_handle_t *handle, uint8_t rom_id, uint8_t fn_cmd, uint16_t len)
{
  assert_param(handle != NULL);
  do
  {
    handle->error = ow_start(handle);
    if (handle->error != OW_ERR_NONE)
    {
      ow_stop(handle);
      break;
    }
    if (len > OW_MAX_DATA_LEN)
    {
      handle->error = OW_ERR_PARAM;
      ow_stop(handle);
      break;
    }
    if (rom_id > handle->rom_max)
    {
      handle->error = OW_ERR_MISS_ROM;
      ow_stop(handle);
      break;
    }
    handle->buf.r_len = len;
    handle->buf.rom_cmd = OW_CMD_SKIP_ROM;
    handle->buf.fn_cmd= fn_cmd;

  } while (0);

  return handle->error;
}
#endif

/*************************************************************************************************/
/**
 */
uint16_t ow_resp_of_read(ow_handle_t *handle, uint8_t *data)
{
  assert_param(handle != NULL);
  if (data != NULL)
  {
    memcpy(data, handle->buf.data, handle->buf.r_len);
  }
  return handle->buf.r_len;
}

/*************************************************************************************************/
/** Private Function Implementations **/
/*************************************************************************************************/

/*************************************************************************************************/
/**
 */
ow_err_t ow_start(ow_handle_t *handle)
{
  ow_err_t ow_err = OW_ERR_NONE;
  do
  {
    if (handle->state != OW_STATE_IDLE)
    {
      ow_err = OW_ERR_BUSY;
      break;
    }
    if (!ow_read_bit(handle))
    {
      ow_err = OW_ERR_BUS;
      break;
    }
    ow_write_bit(handle, true);
    __HAL_TIM_CLEAR_IT(handle->tim_handle, 0xFFFFFFFF);
    memset(&handle->buf, 0, sizeof(ow_buf_t));
    handle->state = OW_STATE_RST;                                               //  change state to reset
    HAL_TIM_Base_Stop_IT(handle->tim_handle);
    __HAL_TIM_SET_COUNTER(handle->tim_handle, 0);
    __HAL_TIM_SET_AUTORELOAD(handle->tim_handle, OW_TIM_RST_DET - 1);
    HAL_TIM_Base_Start_IT(handle->tim_handle);                                  //  start timer

  } while (0);

  return ow_err;
}

/*************************************************************************************************/
/**
 */
void ow_stop(ow_handle_t *handle)
{
  HAL_TIM_Base_Stop_IT(handle->tim_handle);
  ow_write_bit(handle, true);
  handle->state = OW_STATE_IDLE;
}


/*************************************************************************************************/
/**
 */
uint8_t ow_crc(const uint8_t *data, uint16_t len)
{
  uint8_t crc = 0;
  while (len--)
  {
    uint8_t inbyte = *data++;
    for (int i = 8; i > 0; i--)
    {
      uint8_t mix = (crc ^ inbyte) & 0x01;
      crc >>= 1;
      if (mix)
      {
        crc ^= 0x8C;
      }
      inbyte >>= 1;
    }
  }
  return crc;
}

/*************************************************************************************************/
/**
 */
__STATIC_FORCEINLINE void ow_state_rst(ow_handle_t *handle)
{
  switch (handle->buf.tmp)
  {
  case 0:
    __HAL_TIM_SET_AUTORELOAD(handle->tim_handle, OW_TIM_RST - 1);
    ow_write_bit(handle, false);
    handle->buf.tmp++;
    break;
  case 1:
    __HAL_TIM_SET_AUTORELOAD(handle->tim_handle, OW_TIM_RST_DET - 1);
    ow_write_bit(handle, true);
    handle->buf.tmp++;
    break;
  default:
    if (ow_read_bit(handle) != 0)
    {
      ow_stop(handle);
      handle->error = OW_ERR_RESET;
    }
    else
    {
      __HAL_TIM_SET_AUTORELOAD(handle->tim_handle, OW_TIM_RST - 1);
      handle->buf.tmp = 0;
      handle->state = OW_STATE_ROM_CMD;
    }
    break;
  }
}

/*************************************************************************************************/
/**
 */
__STATIC_FORCEINLINE void ow_state_rom_cmd(ow_handle_t *handle)
{
  switch (handle->buf.tmp)
  {
  case 0:
    if (handle->buf.rom_cmd & (1 << handle->buf.bit_i))
    {
      __HAL_TIM_SET_AUTORELOAD(handle->tim_handle, OW_TIM_WRITE_LOW - 1);
    }
    else
    {
      __HAL_TIM_SET_AUTORELOAD(handle->tim_handle, OW_TIM_WRITE_HIGH - 1);
    }
    ow_write_bit(handle, false);
    handle->buf.tmp++;
    break;
  case 1:
    if (handle->buf.rom_cmd & (1 << handle->buf.bit_i))
    {
     __HAL_TIM_SET_AUTORELOAD(handle->tim_handle, OW_TIM_WRITE_HIGH - 1);
    }
    else
    {
     __HAL_TIM_SET_AUTORELOAD(handle->tim_handle, OW_TIM_WRITE_LOW - 1);
    }
    ow_write_bit(handle, true);
    handle->buf.tmp = 0;
    handle->buf.bit_i++;
    if (handle->buf.bit_i == 8)
    {
      handle->buf.bit_i = 0;
      switch (handle->buf.rom_cmd)
      {
      case OW_CMD_MATCH_ROM:
        handle->state = OW_STATE_ROM_ID;
        break;
      case OW_CMD_READ_ROM:
        handle->state = OW_STATE_READ_ROM_ID;
        break;
      case OW_CMD_SEARCH_ROM:
        handle->state = OW_STATE_UPDATE_ROM_ID;
        break;
      case OW_CMD_SEARCH_ALARM:
        handle->state = OW_STATE_UPDATE_ALARM_ID;
        break;
      default:
        handle->state = OW_STATE_FN_CMD;
        break;
      }
    }
    break;
  default:
    break;
  }
}

/*************************************************************************************************/
/**
 */
__STATIC_FORCEINLINE void ow_state_rom_id(ow_handle_t *handle)
{
  switch (handle->buf.tmp)
  {
  case 0:
    if (handle->rom_id[handle->buf.rom_id].rom_id_array[handle->buf.byte_i] & (1 << handle->buf.bit_i))
    {
      __HAL_TIM_SET_AUTORELOAD(handle->tim_handle, OW_TIM_WRITE_LOW - 1);
    }
    else
    {
      __HAL_TIM_SET_AUTORELOAD(handle->tim_handle, OW_TIM_WRITE_HIGH - 1);
    }
    ow_write_bit(handle, false);
    handle->buf.tmp++;
    break;
  case 1:
    if (handle->rom_id[handle->buf.rom_id].rom_id_array[handle->buf.byte_i] & (1 << handle->buf.bit_i))
    {
     __HAL_TIM_SET_AUTORELOAD(handle->tim_handle, OW_TIM_WRITE_HIGH - 1);
    }
    else
    {
     __HAL_TIM_SET_AUTORELOAD(handle->tim_handle, OW_TIM_WRITE_LOW - 1);
    }
    ow_write_bit(handle, true);
    handle->buf.tmp = 0;
    handle->buf.bit_i++;
    if (handle->buf.bit_i == 8)
    {
      handle->buf.bit_i = 0;
      handle->buf.byte_i++;
      if (handle->buf.byte_i == handle->buf.w_len)
      {
        handle->buf.byte_i = 0;
        handle->state = OW_STATE_FN_CMD;
      }
    }
    break;
  default:
    break;
  }
}

/*************************************************************************************************/
/**
 */
__STATIC_FORCEINLINE void ow_state_read_rom_id(ow_handle_t *handle)
{
  switch (handle->buf.tmp)
  {
  case 0:
    __HAL_TIM_SET_AUTORELOAD(handle->tim_handle, OW_TIM_READ_LOW - 1);
    ow_write_bit(handle, false);
    handle->buf.tmp++;
    break;
  case 1:
    __HAL_TIM_SET_AUTORELOAD(handle->tim_handle, OW_TIM_READ_SAMPLE - 1);
    ow_write_bit(handle, true);
    handle->buf.tmp++;
    break;
  case 2:
    __HAL_TIM_SET_AUTORELOAD(handle->tim_handle, OW_TIM_READ_REST - 1);
    handle->buf.tmp = 0;
    if (ow_read_bit(handle))
    {
      handle->rom_id[0].rom_id_array[handle->buf.byte_i] |=  (1 << handle->buf.bit_i);
    }
    handle->buf.bit_i++;
    if (handle->buf.bit_i == 8)
    {
      handle->buf.bit_i = 0;
      handle->buf.byte_i++;
      if (handle->buf.byte_i == 8)
      {
        handle->state = OW_STATE_IDLE;
      }
    }
    break;
  default:
    break;
  }
}

/*************************************************************************************************/
/**
 */
__STATIC_FORCEINLINE void ow_state_fn_cmd(ow_handle_t *handle)
{
  switch (handle->buf.tmp)
  {
  case 0:
    if (handle->buf.fn_cmd & (1 << handle->buf.bit_i))
    {
      __HAL_TIM_SET_AUTORELOAD(handle->tim_handle, OW_TIM_WRITE_LOW - 1);
    }
    else
    {
      __HAL_TIM_SET_AUTORELOAD(handle->tim_handle, OW_TIM_WRITE_HIGH - 1);
    }
    ow_write_bit(handle, false);
    handle->buf.tmp++;
    break;
  case 1:
    if (handle->buf.fn_cmd & (1 << handle->buf.bit_i))
    {
     __HAL_TIM_SET_AUTORELOAD(handle->tim_handle, OW_TIM_WRITE_HIGH - 1);
    }
    else
    {
     __HAL_TIM_SET_AUTORELOAD(handle->tim_handle, OW_TIM_WRITE_LOW - 1);
    }
    ow_write_bit(handle, true);
    handle->buf.tmp = 0;
    handle->buf.bit_i++;
    if (handle->buf.bit_i == 8)
    {
      handle->buf.bit_i = 0;
      if (handle->buf.r_len > 0)
      {
        handle->state = OW_STATE_DATA_R;
      }
      else if (handle->buf.w_len > 0)
      {
        handle->state = OW_STATE_DATA_W;
      }
      else
      {
        handle->state = OW_STATE_IDLE;
      }
    }
    break;
  default:
    break;
  }
}

/*************************************************************************************************/
/**
 */
__STATIC_FORCEINLINE void ow_state_update_rom(ow_handle_t *handle)
{

}

/*************************************************************************************************/
/**
 */
__STATIC_FORCEINLINE void ow_state_update_alarm(ow_handle_t *handle)
{

}

/*************************************************************************************************/
/**
 */
__STATIC_FORCEINLINE void ow_state_data_w(ow_handle_t *handle)
{
  switch (handle->buf.tmp)
  {
  case 0:
    if (handle->buf.data[handle->buf.byte_i] & (1 << handle->buf.bit_i))
    {
      __HAL_TIM_SET_AUTORELOAD(handle->tim_handle, OW_TIM_WRITE_LOW - 1);
    }
    else
    {
      __HAL_TIM_SET_AUTORELOAD(handle->tim_handle, OW_TIM_WRITE_HIGH - 1);
    }
    ow_write_bit(handle, false);
    handle->buf.tmp++;
    break;
  case 1:
    if (handle->buf.data[handle->buf.byte_i] & (1 << handle->buf.bit_i))
    {
     __HAL_TIM_SET_AUTORELOAD(handle->tim_handle, OW_TIM_WRITE_HIGH - 1);
    }
    else
    {
     __HAL_TIM_SET_AUTORELOAD(handle->tim_handle, OW_TIM_WRITE_LOW - 1);
    }
    ow_write_bit(handle, true);
    handle->buf.tmp = 0;
    handle->buf.bit_i++;
    if (handle->buf.bit_i == 8)
    {
      handle->buf.bit_i = 0;
      handle->buf.byte_i++;
      if (handle->buf.byte_i == handle->buf.w_len)
      {
        handle->state = OW_STATE_IDLE;
      }
    }
    break;
  default:
    break;
  }
}

/*************************************************************************************************/
/**
 */
__STATIC_FORCEINLINE void ow_state_data_r(ow_handle_t *handle)
{
  switch (handle->buf.tmp)
  {
  case 0:
    __HAL_TIM_SET_AUTORELOAD(handle->tim_handle, OW_TIM_READ_LOW - 1);
    ow_write_bit(handle, false);
    handle->buf.tmp++;
    break;
  case 1:
    __HAL_TIM_SET_AUTORELOAD(handle->tim_handle, OW_TIM_READ_SAMPLE - 1);
    ow_write_bit(handle, true);
    handle->buf.tmp++;
    break;
  case 2:
    __HAL_TIM_SET_AUTORELOAD(handle->tim_handle, OW_TIM_READ_REST - 1);
    ow_write_bit(handle, true);
    handle->buf.tmp = 0;
    if (ow_read_bit(handle))
    {
      handle->buf.data[handle->buf.byte_i] |=  (1 << handle->buf.bit_i);
    }
    handle->buf.bit_i++;
    if (handle->buf.bit_i == 8)
    {
      handle->buf.bit_i = 0;
      handle->buf.byte_i++;
      if (handle->buf.byte_i == handle->buf.r_len)
      {
        handle->state = OW_STATE_IDLE;
      }
    }
    break;
  default:
    break;
  }
}

/*************************************************************************************************/
/**
 */
__STATIC_FORCEINLINE void ow_write_bit(ow_handle_t *handle, bool high)
{
  if (high)
  {
    handle->gpio->BSRR = handle->pin_set;
  }
  else
  {
    handle->gpio->BSRR = handle->pin_reset;
  }
}

/*************************************************************************************************/
/**
 */
__STATIC_FORCEINLINE uint8_t ow_read_bit(ow_handle_t *handle)
{
  if ((handle->gpio->IDR & handle->pin_read) == handle->pin_read)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

/*************************************************************************************************/
/** End of File **/
/*************************************************************************************************/
