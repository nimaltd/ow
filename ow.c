
/*
 * @file        ow.c
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

/*************************************************************************************************/
/** Includes **/
/*************************************************************************************************/

#include <string.h>
#include "ow.h"
#include "tim.h"

/*************************************************************************************************/
/** Private Function prototype **/
/*************************************************************************************************/

/* Start OneWire communication */
ow_err_t  ow_start(ow_handle_t *handle);

/* Stop OneWire communication */
void      ow_stop(ow_handle_t *handle);

/* Handle transfer state machine */
__STATIC_FORCEINLINE void ow_state_xfer(ow_handle_t *handle);

#if (OW_MAX_DEVICE > 1)
/* Handle search state machine */
__STATIC_FORCEINLINE void ow_state_search(ow_handle_t *handle);
#endif

/* Write one bit on bus */
__STATIC_FORCEINLINE void ow_write_bit(ow_handle_t *handle, bool high);

/* Read one bit from bus */
__STATIC_FORCEINLINE uint8_t ow_read_bit(ow_handle_t *handle);

/*************************************************************************************************/
/** Function Implementations **/
/*************************************************************************************************/

/*************************************************************************************************/
/**
 * @brief  Initialize 1-Wire handle with GPIO and timer configuration.
 * @param  handle: Pointer to the 1-Wire handle to initialize.
 * @param  init: Pointer to initialization data (GPIO, pin, timer, callback).
 * @retval None
 */
void ow_init(ow_handle_t *handle, ow_init_t *init)
{
  assert_param(handle != NULL);
  assert_param(init != NULL);
  assert_param(init->gpio != NULL);
  assert_param(init->tim_handle != NULL);
  assert_param(init->tim_cb != NULL);

  /* Save configuration */
  handle->config.gpio = init->gpio;
  handle->config.pin_set = init->pin;
  handle->config.pin_reset = init->pin << 16UL;
  handle->config.pin_read = init->pin;
  handle->config.tim_handle = init->tim_handle;
  handle->config.done_cb = init->done_cb;

  /* Register user timer callback for timing events */
  HAL_TIM_RegisterCallback(handle->config.tim_handle, HAL_TIM_PERIOD_ELAPSED_CB_ID, init->tim_cb);

  /* Set bus to idle state (high) */
  ow_write_bit(handle, true);
}

/*************************************************************************************************/
/**
 * @brief  Handle 1-Wire timer callback and call state handlers.
 * @param  handle: Pointer to the 1-Wire handle.
 * @retval None
 */
void ow_callback(ow_handle_t *handle)
{
  assert_param(handle != NULL);

  switch (handle->state)
  {
    /* Ongoing data transfer */
    case OW_STATE_XFER:       
      ow_state_xfer(handle);
      break;

#if (OW_MAX_DEVICE > 1)
    /* ROM search operation */
    case OW_STATE_SEARCH:     
      ow_state_search(handle);
      break;
#endif

    /* Any invalid state -> stop */
    default:                  
      ow_stop(handle);
      break;
  }
}

/*************************************************************************************************/
/**
 * @brief  Calculate 8-bit CRC for given data.
 * @param  data: Pointer to the data buffer.
 * @param  len: Length of the data in bytes.
 * @retval Calculated CRC8 value.
 */
uint8_t ow_crc(const uint8_t *data, uint16_t len)
{
  uint8_t crc = 0;
  assert_param(data != NULL);
  assert_param(len > 0);

  while (len--)
  {
    uint8_t inbyte = *data++;
    for (uint8_t i = 8; i > 0; i--)
    {
      /* Compute CRC using polynomial x^8 + x^5 + x^4 + 1 (0x8C) */
      uint8_t mix = (uint8_t)(crc ^ inbyte) & 0x01;
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
 * @brief  Check if 1-Wire bus is busy.
 * @param  handle: Pointer to the 1-Wire handle.
 * @retval true if busy, false if idle
 */
bool ow_is_busy(ow_handle_t *handle)
{
  assert_param(handle != NULL);
  return (handle->state != OW_STATE_IDLE) ? true : false;
}

/*************************************************************************************************/
/**
 * @brief  Get the last 1-Wire error.
 * @param  handle: Pointer to the 1-Wire handle.
 * @retval Last error code (ow_err_t)
 */
ow_err_t ow_last_error(ow_handle_t *handle)
{
  assert_param(handle != NULL);
  return handle->error;
}

#if (OW_MAX_DEVICE == 1)
/*************************************************************************************************/
/**
 * @brief  Start reading a single ROM ID from the 1-Wire bus.
 * @param  handle: Pointer to the 1-Wire handle.
 * @retval Last error code (ow_err_t)
 */
ow_err_t ow_update_rom_id(ow_handle_t *handle)
{
  assert_param(handle != NULL);

  do
  {
    /* Start 1-Wire communication */
    handle->error = ow_start(handle);
    if (handle->error != OW_ERR_NONE)
    {
        /* Stop bus if start failed */
        ow_stop(handle);
        break;
    }

    /* Set state to transfer for next operation */
    handle->state = OW_STATE_XFER;

    /* Prepare buffer to read ROM command (1 byte command + 8 byte response) */
    handle->buf.data[0]   = OW_CMD_READ_ROM;
    handle->buf.write_len = 1;
    handle->buf.read_len  = 8;

  } while (0);

  /* Return result of operation */
  return handle->error;
}
#else

/*************************************************************************************************/
/**
 * @brief  Start search to update all ROM IDs on the 1-Wire bus.
 * @param  handle: Pointer to the 1-Wire handle.
 * @retval Last error code (ow_err_t)
 */
ow_err_t ow_update_rom_id(ow_handle_t *handle)
{
  assert_param(handle != NULL);

  do
  {
    /* Start 1-Wire communication */
    handle->error = ow_start(handle);
    if (handle->error != OW_ERR_NONE)
    {
      /* Stop bus if start failed */
      ow_stop(handle);
      break;
    }

    /* Prepare for ROM search */
    handle->state = OW_STATE_SEARCH;
    handle->buf.data[0] = OW_CMD_SEARCH_ROM;
    handle->rom_id_found = 0;

    /* Clear previous search and ROM ID data */
    memset(&handle->search, 0, sizeof(ow_search_t));
    memset(handle->rom_id, 0, sizeof(handle->rom_id));

  } while (0);

  return handle->error;
}
#endif

/*************************************************************************************************/
/**
 * @brief  Write a command and optional data to any 1-Wire device.
 * @param  handle: Pointer to the 1-Wire handle.
 * @param  fn_cmd: Function/command byte to send.
 * @param  data: Pointer to data buffer (can be NULL).
 * @param  len: Length of data in bytes.
 * @retval Error code from ow_start() or OW_ERR_LEN if data too long.
 */
ow_err_t ow_write_any(ow_handle_t *handle, uint8_t fn_cmd, const uint8_t *data, uint16_t len)
{
  assert_param(handle != NULL);

  do
  {
    /* Check if data length exceeds buffer */
    if (len > OW_MAX_DATA_LEN)
    {
      handle->error = OW_ERR_LEN;
      ow_stop(handle);
      break;
    }

    /* Start 1-Wire communication */
    handle->error = ow_start(handle);
    if (handle->error != OW_ERR_NONE)
    {
      ow_stop(handle);
      break;
    }

    /* Prepare transfer buffer */
    handle->state = OW_STATE_XFER;

    /* Send SKIP ROM command */
    handle->buf.data[0] = OW_CMD_SKIP_ROM;

    /* Send function command */
    handle->buf.data[1] = fn_cmd;

    /* Copy user data if provided */
    if (data != NULL)
    {
      for (uint16_t idx = 0; idx < len; idx++)
      {
        handle->buf.data[2 + idx] = data[idx];
      }
      handle->buf.write_len = len + 2;
    }
    else
    {
      handle->buf.write_len = 2;
    }

  } while (0);

  return handle->error;
}

/*************************************************************************************************/
/**
 * @brief  Read data from any 1-Wire device.
 * @param  handle: Pointer to the 1-Wire handle.
 * @param  fn_cmd: Function/command byte to send.
 * @param  len: Number of bytes to read.
 * @retval Error code from ow_start() or OW_ERR_LEN if requested length too long.
 */
ow_err_t ow_read_any(ow_handle_t *handle, uint8_t fn_cmd, uint16_t len)
{
  assert_param(handle != NULL);

  do
  {
    /* Check if requested read length exceeds buffer */
    if (len > OW_MAX_DATA_LEN)
    {
      handle->error = OW_ERR_LEN;
      ow_stop(handle);
      break;
    }

    /* Start 1-Wire communication */
    handle->error = ow_start(handle);
    if (handle->error != OW_ERR_NONE)
    {
      ow_stop(handle);
      break;
    }

    /* Prepare transfer buffer */
    handle->state = OW_STATE_XFER;

    /* Skip ROM for single device */
    handle->buf.data[0] = OW_CMD_SKIP_ROM;

    /* Send function command */
    handle->buf.data[1] = fn_cmd;
    handle->buf.write_len = 2;

    /* Set expected read length */
    handle->buf.read_len  = len;

  } while (0);

  return handle->error;
}

#if (OW_MAX_DEVICE > 1)
/*************************************************************************************************/
/**
 * @brief  Write a command and optional data to a specific 1-Wire device by ROM ID index.
 * @param  handle: Pointer to the 1-Wire handle.
 * @param  rom_id: Index of the target ROM ID in handle->rom_id array.
 * @param  fn_cmd: Function/command byte to send.
 * @param  data: Pointer to data buffer (can be NULL).
 * @param  len: Length of data in bytes.
 * @retval Error code (OW_ERR_NONE on success, OW_ERR_LEN, OW_ERR_ROM_ID, etc.)
 */
ow_err_t ow_write_by_id(ow_handle_t *handle, uint8_t rom_id, uint8_t fn_cmd, const uint8_t *data, uint16_t len)
{
  assert_param(handle != NULL);

  do
  {
    /* Check if data length exceeds buffer */
    if (len > OW_MAX_DATA_LEN)
    {
      handle->error = OW_ERR_LEN;
      ow_stop(handle);
      break;
    }

    /* Validate ROM ID index */
    if ((handle->rom_id_found == 0) || (rom_id >= handle->rom_id_found))
    {
      handle->error = OW_ERR_ROM_ID;
      ow_stop(handle);
      break;
    }

    /* Start 1-Wire communication */
    handle->error = ow_start(handle);
    if (handle->error != OW_ERR_NONE)
    {
      ow_stop(handle);
      break;
    }

    /* Prepare transfer buffer */
    handle->state = OW_STATE_XFER;
    
    /* Select device by ROM */
    handle->buf.data[0] = OW_CMD_MATCH_ROM;
    for (uint8_t idx = 0; idx < 8; idx++)
    {
      handle->buf.data[1 + idx] = handle->rom_id[rom_id].rom_id_array[idx];
    }
    
    /* Function command */
    handle->buf.data[9] = fn_cmd;

    /* Copy user data if provided */
    if ((data != NULL) && (len > 0))
    {
      for (uint16_t idx = 0; idx < len; idx++)
      {
        handle->buf.data[10 + idx] = data[idx];
      }
      handle->buf.write_len = len + 10;
    }
    else
    {
      handle->buf.write_len = 10;
    }

  } while (0);

  return handle->error;
}

/*************************************************************************************************/
/**
 * @brief  Read data from a specific 1-Wire device by ROM ID index.
 * @param  handle: Pointer to the 1-Wire handle.
 * @param  rom_id: Index of the target ROM ID in handle->rom_id array.
 * @param  fn_cmd: Function/command byte to send.
 * @param  len: Number of bytes to read.
 * @retval Error code (OW_ERR_NONE on success, OW_ERR_LEN, OW_ERR_ROM_ID, etc.)
 */
ow_err_t ow_read_by_id(ow_handle_t *handle, uint8_t rom_id, uint8_t fn_cmd, uint16_t len)
{
  assert_param(handle != NULL);

  do
  {
    /* Check if requested read length exceeds buffer */
    if (len > OW_MAX_DATA_LEN)
    {
      handle->error = OW_ERR_LEN;
      ow_stop(handle);
      break;
    }

    /* Validate ROM ID index */
    if ((handle->rom_id_found == 0) || (rom_id >= handle->rom_id_found))
    {
      handle->error = OW_ERR_ROM_ID;
      ow_stop(handle);
      break;
    }

    /* Start 1-Wire communication */
    handle->error = ow_start(handle);
    if (handle->error != OW_ERR_NONE)
    {
      ow_stop(handle);
      break;
    }

    /* Prepare transfer buffer */
    handle->state = OW_STATE_XFER;

    /* Select device by ROM */
    handle->buf.data[0] = OW_CMD_MATCH_ROM;
    for (int idx = 0; idx < 8; idx++)
    {
      handle->buf.data[1 + idx] = handle->rom_id[rom_id].rom_id_array[idx];
    }
    
    /* Function command */
    handle->buf.data[9] = fn_cmd;

    /* Total bytes to write */
    handle->buf.write_len = 10;

    /* Number of bytes to read */
    handle->buf.read_len = len;

  } while (0);

  return handle->error;
}

/*************************************************************************************************/
/**
 * @brief  Get number of detected 1-Wire devices.
 * @param  handle: Pointer to 1-Wire handle.
 * @retval Count of found devices
 */
uint8_t ow_devices(ow_handle_t *handle)
{
  assert_param(handle != NULL);
  return handle->rom_id_found;
}
#endif

/*************************************************************************************************/
/**
 * @brief  Retrieve read response data from the 1-Wire buffer.
 * @param  handle: Pointer to the 1-Wire handle.
 * @param  data: Pointer to user buffer to store the response.
 * @param  data_size: Size of the user buffer in bytes.
 * @retval Number of bytes copied to the user buffer.
 */
uint16_t ow_read_resp(ow_handle_t *handle, uint8_t *data, uint16_t data_size)
{
  assert_param(handle != NULL);
  assert_param(data != NULL);

  uint16_t len = handle->buf.read_len;

  /* Adjust length if user buffer is smaller */
  if (data_size < len)
  {
    len = data_size;
  }

  /* Defensive: ensure we do not read past internal buffer */
  if ((uint32_t)(handle->buf.write_len) + (uint32_t)len > sizeof(handle->buf.data))
  {
    /* Truncate to available bytes */
    if (handle->buf.write_len < sizeof(handle->buf.data))
    {
      len = (uint16_t)(sizeof(handle->buf.data) - (uint32_t)handle->buf.write_len);
    }
    else
    {
      return 0U;
    }
  }

  /* Copy response data from internal buffer to user buffer */
  for (uint16_t idx = 0U; idx < len; ++idx)
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
 * @brief  Start a 1-Wire transfer on the bus.
 * @param  handle: Pointer to the 1-Wire handle.
 * @retval OW_ERR_NONE on success, or error code (OW_ERR_BUSY, OW_ERR_BUS).
 */
ow_err_t ow_start(ow_handle_t *handle)
{
  ow_err_t ow_err = OW_ERR_NONE;
  assert_param(handle != NULL);

  do
  {
    /* Ensure bus is idle before starting transfer */
    if (handle->state != OW_STATE_IDLE)
    {
      ow_err = OW_ERR_BUSY;
      break;
    }

    /* Pull bus high and check if line is idle */
    ow_write_bit(handle, true);
    if (!ow_read_bit(handle))
    {
      ow_err = OW_ERR_BUS;
      break;
    }

    /* Clear timer interrupt and reset internal buffer */
    __HAL_TIM_CLEAR_IT(handle->config.tim_handle, 0xFFFFFFFFUL);
    memset(&handle->buf, 0, sizeof(ow_buf_t));

    /* Configure timer for reset detection */
    __HAL_TIM_SET_COUNTER(handle->config.tim_handle, 0);
    __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_RST_DET - 1);
    HAL_TIM_Base_Start_IT(handle->config.tim_handle);

  } while (0);

  return ow_err;
}

/*************************************************************************************************/
/**
 * @brief  Stop 1-Wire transfer and release the bus.
 * @param  handle Pointer to the 1-Wire handle.
 */
void ow_stop(ow_handle_t *handle)
{
  assert_param(handle != NULL);

  /* Stop timer interrupts */
  HAL_TIM_Base_Stop_IT(handle->config.tim_handle);

  /* Release bus (set high) */
  ow_write_bit(handle, true);

  /* Set state to idle */
  handle->state = OW_STATE_IDLE;

  /* Call user callback if registered */
  if (handle->config.done_cb != NULL)
  {
    handle->config.done_cb(handle->error);
  }
}

/*************************************************************************************************/
/**
 * @brief  1-Wire state machine: handle transfer phases (reset, write, read).
 * @param  handle Pointer to the 1-Wire handle.
 */
__STATIC_FORCEINLINE void ow_state_xfer(ow_handle_t *handle)
{
  assert_param(handle != NULL);

  switch (handle->buf.bit_ph)
  {
    /************ Reset phase: pull bus low ************/
    case 0:
      __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_RST - 1);
      ow_write_bit(handle, false);
      handle->buf.bit_ph++;
      break;

    /************ Reset phase: release bus (high) ************/
    case 1:
      __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_RST_DET - 1);
      ow_write_bit(handle, true);
      handle->buf.bit_ph++;
      break;

    /************ Reset phase: check presence pulse ************/
    case 2:
      if (ow_read_bit(handle) != 0)
      {
        handle->error = OW_ERR_RESET;
        ow_stop(handle);
      }
      else
      {
        __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_RST - 1);
        handle->buf.bit_ph++;
      }
      break;

    /************ Writing, phase 1: pull low ************/
    case 3:
      __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle,
        (handle->buf.data[handle->buf.byte_idx] & (1 << handle->buf.bit_idx)) ? OW_TIM_WRITE_LOW - 1 : OW_TIM_WRITE_HIGH - 1);
      ow_write_bit(handle, false);
      handle->buf.bit_ph++;
      break;

    /************ Writing, phase 2: release bus ************/
    case 4:
      __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle,
        (handle->buf.data[handle->buf.byte_idx] & (1 << handle->buf.bit_idx)) ? OW_TIM_WRITE_HIGH - 1 : OW_TIM_WRITE_LOW - 1);
      ow_write_bit(handle, true);
      handle->buf.bit_idx++;

      /* Move to next byte or reading phase */
      if (handle->buf.bit_idx == 8)
      {
        handle->buf.bit_idx = 0;
        handle->buf.byte_idx++;
        if (handle->buf.byte_idx == handle->buf.write_len)
        {
          if (handle->buf.read_len > 0)
          {
            /* Start reading phase */
            handle->buf.bit_ph = 5;
            handle->buf.byte_idx = 0;
          }
          else
          {
            /* Writing complete, no reading */
            handle->state = OW_STATE_DONE;
          }
        }
        else
        {
          /* Continue writing next byte */
          handle->buf.bit_ph = 3;
        }
      }
      else
      {
        /* Continue writing next bit */
        handle->buf.bit_ph = 3;
      }
      break;

    /************ Reading, phase 1: pull low ************/
    case 5:
      __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_READ_LOW - 1);
      ow_write_bit(handle, false);
      handle->buf.bit_ph++;
      break;

    /************ Reading, phase 2: release bus ************/
    case 6:
      __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_READ_SAMPLE - 1);
      ow_write_bit(handle, true);
      handle->buf.bit_ph++;
      break;

    /************ Reading, phase 3: sample bus ************/
    case 7:
      __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_READ_HIGH - 1);
      if (ow_read_bit(handle))
      {
        handle->buf.data[handle->buf.write_len + handle->buf.byte_idx] |= (1 << handle->buf.bit_idx);
      }

      /* Update bit/byte counters */
      handle->buf.bit_ph = 5;
      handle->buf.bit_idx++;
      if (handle->buf.bit_idx == 8)
      {
        handle->buf.bit_idx = 0;
        handle->buf.byte_idx++;
        if (handle->buf.byte_idx == handle->buf.read_len)
        {
#if (OW_MAX_DEVICE == 1)
          /* Single device: verify ROM ID if READ_ROM command */
          if (handle->buf.data[0] == OW_CMD_READ_ROM)
          {
            if (ow_crc(&handle->buf.data[1], 7) == handle->buf.data[7])
            {
              memcpy(handle->rom_id[0].rom_id_array, &handle->buf.data[1], 8);
              handle->error = OW_ERR_NONE;
            }
            else
            {
              handle->error = OW_ERR_ROM_ID;
            }
          }
#endif
          handle->state = OW_STATE_DONE;
        }
      }
      break;

    default:
      break;
  }
}

#if (OW_MAX_DEVICE > 1)
/*************************************************************************************************/
/**
 * @brief  1-Wire ROM search state machine.
 * @param  handle: Pointer to 1-Wire handle.
 * @retval None
 *
 * @details
 * Implements non-blocking search, handles reset, read/write bits,
 * resolves discrepancies, stores found ROMs, and sets state done.
 */
__STATIC_FORCEINLINE void ow_state_search(ow_handle_t *handle)
{
  assert_param(handle != NULL);

  switch (handle->buf.bit_ph)
  {
  /************ Reset phase: pull bus low ************/
  case 0:
    __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_RST - 1);
    ow_write_bit(handle, false);
    handle->buf.bit_ph++;
    break;

    /************ Reset phase: release bus (high) ************/
  case 1:
    __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_RST_DET - 1);
    ow_write_bit(handle, true);
    handle->buf.bit_ph++;
    break;

  /************ Reset phase: check presence pulse ************/
  case 2:
    if (ow_read_bit(handle) != 0)
    {
      handle->error = OW_ERR_RESET;
      ow_stop(handle);
    }
    else
    {
      __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_RST - 1);
      handle->buf.bit_ph++;
    }
    break;

  /************ Writing, phase 1: pull low ************/
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

  /************ Writing, phase 2: release bus ************/
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

    /* command complete */
    if (handle->buf.bit_idx == 8)
    {
      handle->buf.bit_idx = 0;
      /* Start reading phase */
      handle->buf.bit_ph = 5;
    }
    else
    {
      /* Writing next command bit */
      handle->buf.bit_ph = 3;
    }
    break;

  /************ Reading, phase 1: pull low ************/
  case 5:
    __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_READ_LOW - 1);
    ow_write_bit(handle, false);
    handle->buf.bit_ph++;
    break;

  /************ reading bit, phase 2 ************/
  case 6:
    __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_READ_SAMPLE - 1);
    ow_write_bit(handle, true);
    handle->buf.bit_ph++;
    break;

  /************ reading bit, phase 3 ************/
  case 7:
    __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_READ_HIGH - 1);
    if (ow_read_bit(handle))
    {
      handle->search.val = OW_VAL_1;
    }
    else
    {
      handle->search.val = OW_VAL_DIFF;
    }
    handle->buf.bit_ph++;
    break;

  /************ reading complement bit, phase 1 ************/
  case 8:
    __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_READ_LOW - 1);
    ow_write_bit(handle, false);
    handle->buf.bit_ph++;
    break;

  /************ Reading, phase 2: release bus ************/
  case 9:
    __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_READ_SAMPLE - 1);
    ow_write_bit(handle, true);
    handle->buf.bit_ph++;
    break;

  /************ Reading, phase 3: sample bus ************/
  case 10:
    __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_READ_HIGH - 1);
    if (ow_read_bit(handle))
    {
      handle->search.val |= OW_VAL_0;
    }
    handle->buf.bit_ph++;

    /* resolve discrepancy */
    /* Dallas counts bits 1..64 */
    uint8_t bit_number = handle->buf.bit_idx + 1;
    if (handle->search.val == OW_VAL_DIFF)
    {
      uint8_t bit_choice = 0;
      if (bit_number < handle->search.last_discrepancy)
      {
        /* repeat previous path */
        bit_choice = (handle->search.rom_id[handle->buf.bit_idx / 8] >> (handle->buf.bit_idx % 8)) & 0x01;
      }
      else if (bit_number == handle->search.last_discrepancy)
      {
        /* this time choose 1 */
        bit_choice = 1;
      }
      else
      {
        /* choose 0 and remember as last zero */
        bit_choice = 0;
        handle->search.last_zero = bit_number;
      }
      handle->search.val = bit_choice ? OW_VAL_1 : OW_VAL_0;
    }
    else if (handle->search.val == OW_VAL_ERR)
    {
      handle->error = OW_ERR_ROM_ID;
      ow_stop(handle);
    }
    break;

  /************ Writing selected bit, phase 1: pull low ************/
  case 11:
    if (handle->search.val == OW_VAL_1)
    {
      __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_WRITE_LOW - 1);
      handle->search.rom_id[handle->buf.bit_idx / 8] |= (1 << (handle->buf.bit_idx % 8));
    }
    else
    {
      __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_WRITE_HIGH - 1);
    }
    ow_write_bit(handle, false);
    handle->buf.bit_ph++;
    break;

  /************ Writing selected bit, phase 2: release bus ************/
  case 12:
    if (handle->search.val == OW_VAL_1)
    {
      __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_WRITE_HIGH - 1);
    }
    else
    {
      __HAL_TIM_SET_AUTORELOAD(handle->config.tim_handle, OW_TIM_WRITE_LOW - 1);
    }
    ow_write_bit(handle, true);
    handle->buf.bit_idx++;
    if (handle->buf.bit_idx == 64)
    {
      /* full ROM read */
      handle->buf.bit_idx = 0;
      handle->buf.bit_ph = 0;
      if (ow_crc(handle->search.rom_id, 7) == handle->search.rom_id[7])
      {
        memcpy(&handle->rom_id[handle->rom_id_found], handle->search.rom_id, 8);
        handle->rom_id_found++;
      }
      memset(handle->search.rom_id, 0, 8);

      /* update discrepancy */
      handle->search.last_discrepancy = handle->search.last_zero;
      handle->search.last_zero = 0;
      if (handle->search.last_discrepancy == 0 || handle->rom_id_found == OW_MAX_DEVICE)
      {
        handle->search.last_device_flag = 1;
        handle->state = OW_STATE_DONE;
      }
      else
      {
        /* prepare next search */
        handle->buf.data[0] = OW_CMD_SEARCH_ROM;
        handle->buf.bit_idx = 0;
        handle->buf.bit_ph = 0;
      }
    }
    else
    {
      /* next search bit */
      handle->buf.bit_ph = 5;
    }
    break;
  default:
    break;
  }
}
#endif

/*************************************************************************************************/
/**
 * @brief  Set 1-Wire bus pin high or low.
 * @param  handle: Pointer to 1-Wire handle.
 * @param  high: true to set high, false to set low.
 * @retval None
 */
__STATIC_FORCEINLINE void ow_write_bit(ow_handle_t *handle, bool high)
{
  assert_param(handle != NULL);

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
 * @brief  Read current level of 1-Wire bus pin.
 * @param  handle: Pointer to 1-Wire handle.
 * @retval 1 if high, 0 if low
 */
__STATIC_FORCEINLINE uint8_t ow_read_bit(ow_handle_t *handle)
{
  assert_param(handle != NULL);

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
