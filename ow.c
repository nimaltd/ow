

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

__STATIC_FORCEINLINE void ow_state_xfer(ow_handle_t *handle);
__STATIC_FORCEINLINE void ow_state_search(ow_handle_t *handle);
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
  handle->config.gpio = init->gpio;
  handle->config.pin_set = init->pin;
  handle->config.pin_reset = init->pin << 16UL;
  handle->config.pin_read = init->pin;
  handle->config.tim_handle = init->tim_handle;
  HAL_TIM_RegisterCallback(handle->config.tim_handle, HAL_TIM_PERIOD_ELAPSED_CB_ID, init->tim_cb);
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
    case OW_STATE_XFER:
      ow_state_xfer(handle);
      break;
    case OW_STATE_SEARCH:
      ow_state_search(handle);
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
ow_err_t ow_read_single_rom_id(ow_handle_t *handle)
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
    handle->state = OW_STATE_XFER;
    handle->buf.data[0] = OW_CMD_READ_ROM;
    handle->buf.write_len = 1;
    handle->buf.read_len = 8;

  } while (0);

  return handle->error;
}

/*************************************************************************************************/
/**
 */
ow_err_t ow_write(ow_handle_t *handle, uint8_t fn_cmd, const uint8_t *data, uint16_t len)
{
  assert_param(handle != NULL);
  assert_param(data != NULL);
  do
  {
    if (len > OW_MAX_DATA_LEN)
    {
      ow_stop(handle);
      handle->error = OW_ERR_LEN;
      break;
    }
    handle->error = ow_start(handle);
    if (handle->error != OW_ERR_NONE)
    {
      ow_stop(handle);
      break;
    }
    handle->state = OW_STATE_XFER;
    handle->buf.data[0] = OW_CMD_SKIP_ROM;
    handle->buf.data[1] = fn_cmd;
    handle->buf.write_len = len + 2;

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
    if (len > OW_MAX_DATA_LEN)
    {
      ow_stop(handle);
      handle->error = OW_ERR_LEN;
      break;
    }
    handle->error = ow_start(handle);
    if (handle->error != OW_ERR_NONE)
    {
      ow_stop(handle);
      break;
    }
    handle->state = OW_STATE_XFER;
    handle->buf.data[0] = OW_CMD_SKIP_ROM;
    handle->buf.data[1] = fn_cmd;
    handle->buf.write_len = 2;
    handle->buf.read_len = len;

  } while (0);

  return handle->error;
}


#if (OW_MAX_DEVICE > 1)
/*************************************************************************************************/
/**
 */
ow_err_t ow_update_all_rom_id(ow_handle_t *handle)
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
    handle->state = OW_STATE_SEARCH;
    handle->buf.data[0] = OW_CMD_SEARCH_ROM;
    handle->rom_id_found = 0;
    handle->search_val = 0;
    handle->search_diff_idx = -1;
    memset(handle->rom_id, 0, sizeof(handle->rom_id));

  } while (0);

  return handle->error;
}

/*************************************************************************************************/
/**
 */
ow_err_t ow_write_by_id(ow_handle_t *handle, uint8_t rom_id, uint8_t fn_cmd, const uint8_t *data, uint16_t len)
{
  assert_param(handle != NULL);
  assert_param(data != NULL);
  do
  {
    if (len > OW_MAX_DATA_LEN)
    {
      ow_stop(handle);
      handle->error = OW_ERR_LEN;
      break;
    }
    if ((handle->rom_id_found == 0) || (rom_id >= handle->rom_id_found))
    {
      ow_stop(handle);
      handle->error = OW_ERR_ROM_ID;
      break;
    }
    handle->error = ow_start(handle);
    if (handle->error != OW_ERR_NONE)
    {
      ow_stop(handle);
      break;
    }
    handle->state = OW_STATE_XFER;
    handle->buf.data[0] = OW_CMD_MATCH_ROM;
    handle->buf.data[1] = fn_cmd;
    for (int idx = 0; idx < 8; idx++)
    {
      handle->buf.data[2 + idx] = handle->rom_id[rom_id].rom_id_array[idx];
    }
    handle->buf.write_len = len + 10 ;

  } while (0);

  return handle->error;
}

/*************************************************************************************************/
/**
 */
ow_err_t ow_read_by_id(ow_handle_t *handle, uint8_t rom_id, uint8_t fn_cmd, uint16_t len)
{
  assert_param(handle != NULL);
  do
  {
    if (len > OW_MAX_DATA_LEN)
    {
      ow_stop(handle);
      handle->error = OW_ERR_LEN;
      break;
    }
    if ((handle->rom_id_found == 0) || (rom_id >= handle->rom_id_found))
    {
      ow_stop(handle);
      handle->error = OW_ERR_ROM_ID;
      break;
    }
    handle->error = ow_start(handle);
    if (handle->error != OW_ERR_NONE)
    {
      ow_stop(handle);
      break;
    }
    handle->state = OW_STATE_XFER;
    handle->buf.data[0] = OW_CMD_MATCH_ROM;
    handle->buf.data[1] = fn_cmd;
    for (int idx = 0; idx < 8; idx++)
    {
      handle->buf.data[2 + idx] = handle->rom_id[rom_id].rom_id_array[idx];
    }
    handle->buf.write_len = 10;
    handle->buf.read_len = len;

  } while (0);

  return handle->error;
}
#endif

/*************************************************************************************************/
/**
 */
uint16_t ow_read_resp(ow_handle_t *handle, uint8_t *data, uint16_t data_size)
{
  uint16_t len = handle->buf.read_len;
  assert_param(handle != NULL);
  assert_param(data != NULL);
  if (data_size < len)
  {
    len = data_size;
  }
  for (uint16_t idx = 0; idx < len; idx++)
  {
    data[idx] = handle->buf.data[handle->buf.write_len + idx];
  }
  return len;
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
    __HAL_TIM_CLEAR_IT(handle->config.tim_handle, 0xFFFFFFFF);
    memset(&handle->buf, 0, sizeof(ow_buf_t));
    __HAL_TIM_SET_COUNTER(handle->config.tim_handle, 0);
    __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_RST_DET - 1);
    HAL_TIM_Base_Start_IT(handle->config.tim_handle);

  } while (0);

  return ow_err;
}

/*************************************************************************************************/
/**
 */
void ow_stop(ow_handle_t *handle)
{
  HAL_TIM_Base_Stop_IT(handle->config.tim_handle);
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
__STATIC_FORCEINLINE void ow_state_xfer(ow_handle_t *handle)
{
  switch (handle->buf.bit_ph)
  {
  /************ reset phase, set pin to 0 ************/
  case 0:
    __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_RST - 1);
    ow_write_bit(handle, false);
    handle->buf.bit_ph++;
    break;
  /************ reset phase, set pin to 1 ************/
  case 1:
    __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_RST_DET - 1);
    ow_write_bit(handle, true);
    handle->buf.bit_ph++;
    break;
  /************ reset phase, read pin ************/
  case 2:
    if (ow_read_bit(handle) != 0)
    {
      ow_stop(handle);
      handle->error = OW_ERR_RESET;
    }
    else
    {
      __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_RST - 1);
      handle->buf.bit_ph++;
    }
    break;
  /************ writing phase 1 ************/
  case 3:
    if (handle->buf.data[handle->buf.byte_idx] & (1 << handle->buf.bit_idx))
    {
      __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_WRITE_LOW - 1);
    }
    else
    {
      __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_WRITE_HIGH - 1);
    }
    ow_write_bit(handle, false);
    handle->buf.bit_ph++;
    break;
  /************ writing phase 2 ************/
  case 4:
    if (handle->buf.data[handle->buf.byte_idx] & (1 << handle->buf.bit_idx))
    {
      __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_WRITE_HIGH - 1);
    }
    else
    {
      __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_WRITE_LOW - 1);
    }
    ow_write_bit(handle, true);
    handle->buf.bit_idx++;
    // a byte sent
    if (handle->buf.bit_idx == 8)
    {
      handle->buf.bit_idx = 0;
      handle->buf.byte_idx++;
      // all data has been written
      if (handle->buf.byte_idx == handle->buf.write_len)
      {
        // check for reading
        if (handle->buf.read_len > 0)
        {
          // jump to reading phase
          handle->buf.bit_ph = 5;
          handle->buf.byte_idx = 0;
        }
        // no need to read
        else
        {
          handle->state = OW_STATE_DONE;
        }
      }
    }
    else
    {
      // jump to write bit phase
      handle->buf.bit_ph = 3;
    }
    break;
  /************ reading phase 1 ************/
  case 5:
    __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_READ_LOW - 1);
    ow_write_bit(handle, false);
    handle->buf.bit_ph++;
    break;
  /************ reading phase 2 ************/
  case 6:
    __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_READ_SAMPLE - 1);
    ow_write_bit(handle, true);
    handle->buf.bit_ph++;
    break;
  /************ reading phase 3 ************/
  case 7:
    __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_READ_HIGH - 1);
    if (ow_read_bit(handle))
    {
      handle->buf.data[handle->buf.write_len + handle->buf.byte_idx] |=  (1 << handle->buf.bit_idx);
    }
    // jump to read phase
    handle->buf.bit_ph = 5;
    handle->buf.bit_idx++;
    // a byte read
    if (handle->buf.bit_idx == 8)
    {
      handle->buf.bit_idx = 0;
      handle->buf.byte_idx++;
      // all data has been read
      if (handle->buf.byte_idx == handle->buf.read_len)
      {
        handle->state = OW_STATE_DONE;
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
__STATIC_FORCEINLINE void ow_state_search(ow_handle_t *handle)
{
  switch (handle->buf.bit_ph)
  {
  /************ reset phase, set pin to 0 ************/
  case 0:
    __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_RST - 1);
    ow_write_bit(handle, false);
    handle->buf.bit_ph++;
    break;
  /************ reset phase, set pin to 1 ************/
  case 1:
    __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_RST_DET - 1);
    ow_write_bit(handle, true);
    handle->buf.bit_ph++;
    break;
  /************ reset phase, read pin ************/
  case 2:
    if (ow_read_bit(handle) != 0)
    {
      ow_stop(handle);
      handle->error = OW_ERR_RESET;
    }
    else
    {
      __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_RST - 1);
      handle->buf.bit_ph++;
    }
    break;
  /************ writing phase 1 ************/
  case 3:
    if (handle->buf.data[0] & (1 << handle->buf.bit_idx))
    {
      __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_WRITE_LOW - 1);
    }
    else
    {
      __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_WRITE_HIGH - 1);
    }
    ow_write_bit(handle, false);
    handle->buf.bit_ph++;
    break;
  /************ writing phase 2 ************/
  case 4:
    if (handle->buf.data[0] & (1 << handle->buf.bit_idx))
    {
      __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_WRITE_HIGH - 1);
    }
    else
    {
      __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_WRITE_LOW - 1);
    }
    ow_write_bit(handle, true);
    handle->buf.bit_idx++;
    // a byte sent (CMD)
    if (handle->buf.bit_idx == 8)
    {
      handle->buf.bit_idx = 0;
      // jump to searching phase
      handle->buf.bit_ph = 5;
    }
    else
    {
      // jump to write bit phase
      handle->buf.bit_ph = 3;
    }
    break;
  /************ reading first bit, phase 1 ************/
  case 5:
    __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_READ_LOW - 1);
    ow_write_bit(handle, false);
    handle->buf.bit_ph++;
    break;
  /************ reading first bit, phase 2 ************/
  case 6:
    __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_READ_SAMPLE - 1);
    ow_write_bit(handle, true);
    handle->buf.bit_ph++;
    break;
  /************ reading first bit, phase 3 ************/
  case 7:
    __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_READ_HIGH - 1);
    if (ow_read_bit(handle))
    {
      handle->search_val = 0x01;
    }
    else
    {
      handle->search_val = 0x00;
    }
    handle->buf.bit_ph++;
    break;
  /************ reading second bit, phase 1 ************/
  case 8:
    __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_READ_LOW - 1);
    ow_write_bit(handle, false);
    handle->buf.bit_ph++;
    break;
  /************ reading second bit, phase 2 ************/
  case 9:
    __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_READ_SAMPLE - 1);
    ow_write_bit(handle, true);
    handle->buf.bit_ph++;
    break;
  /************ reading second bit, phase 3 ************/
  case 10:
    __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_READ_HIGH - 1);
    if (ow_read_bit(handle))
    {
      handle->search_val |= 0x10;
    }
    handle->buf.bit_ph++;
    // found difference
    if (handle->search_val == 0x00)
    {
      if ((handle->search_found == 0) && (handle->buf.bit_idx > handle->search_diff_idx))
      {
        handle->search_found = 1;
        handle->search_diff_idx = handle->buf.bit_idx;
        handle->search_val = 0x10;
      }
      else if ((handle->search_found == 1) && (handle->buf.bit_idx == handle->search_diff_idx))
      {
        handle->search_found = 0;
        handle->search_val = 0x01;
      }
    }
    // error
    else if (handle->search_val == 0x11)
    {
      ow_stop(handle);
      handle->error = OW_ERR_ROM_ID;
    }
    break;
  /************ writing selected bit, phase 1 ************/
  case 11:
    if (handle->search_val == 0x01)
    {
      __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_WRITE_LOW - 1);
      handle->buf.data[1 + (handle->buf.bit_idx / 8)] |= (1 << (handle->buf.bit_idx % 8));
    }
    else
    {
      __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_WRITE_HIGH - 1);
    }
    ow_write_bit(handle, false);
    handle->buf.bit_ph++;
    break;
  /************ writing selected bit, phase 2 ************/
  case 12:
    if (handle->search_val == 0x01)
    {
      __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_WRITE_HIGH - 1);
    }
    else
    {
      __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_WRITE_LOW - 1);
    }
    ow_write_bit(handle, true);
    handle->buf.bit_idx++;
    // all bytes of a ROM has been read
    if (handle->buf.bit_idx == 64)
    {
      handle->buf.bit_idx = 0;
      handle->buf.bit_ph = 0;
      if (ow_crc(&handle->buf.data[1], 7) == handle->buf.data[8])
      {
        memcpy(&handle->rom_id[handle->rom_id_found], &handle->buf.data[1], 8);
        handle->rom_id_found++;
      }
      // reset buffer
      memset(&handle->buf.data[1], 0, sizeof(handle->buf.data) - 1);

      if (handle->rom_id_found == OW_MAX_DEVICE)
      {
        handle->state = OW_STATE_DONE;
      }
    }
    else
    {
      // jump to searching phase
      handle->buf.bit_ph = 5;
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
    handle->config.gpio->BSRR = handle->config.pin_set;
  }
  else
  {
    handle->config.gpio->BSRR = handle->config.pin_reset;
  }
}

/*************************************************************************************************/
/**
 */
__STATIC_FORCEINLINE uint8_t ow_read_bit(ow_handle_t *handle)
{
  if ((handle->config.gpio->IDR & handle->config.pin_read) == handle->config.pin_read)
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
