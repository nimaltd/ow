# Non-Blocking 1-Wire Library for STM32  
---  
## Please Do not Forget to get STAR, DONATE and support me on social networks. Thank you. :sparkling_heart:  
---   
-  Author:     Nima Askari  
-  Github:     https://www.github.com/NimaLTD
-  Youtube:    https://www.youtube.com/@nimaltd  
-  LinkedIn:   https://www.linkedin.com/in/nimaltd  
-  Instagram:  https://instagram.com/github.NimaLTD  
---


A lightweight and efficient **One Wire (1-Wire)** protocol library written in C for STM32 (HAL-based).  
It provides full support for 1-Wire devices such as **DS18B20 temperature sensors**, with multi-device support, asynchronous handling, and easy integration with STM32 projects.  

---

## ✨ Features

- 🔹 Supports single or multiple 1-Wire devices  
- 🔹 Fully compatible with STM32 HAL (`tim.h`, `gpio`)  
- 🔹 ROM ID management (read, match, skip, search)  
- 🔹 Error handling (bus, reset, ROM ID, length)  
- 🔹 Non-blocking operation using timer callbacks  
- 🔹 Simple and modular API  

---

## 📂 Project Structure

```
├── ow.h             # Main header file (API definitions)
├── ow.c             # Implementation (if separated)
├── ow_config.h      # User configuration (max devices, buffer size, etc.)
└── examples/        # Example STM32 projects
```

---

## ⚙️ Installation

1. Copy the following files into your STM32 project:
   - `ow.h`
   - `ow.c`
   - `ow_config.h`

2. Include the library in your project:

```c
#include "ow.h"
```

3. Configure your **GPIO pin** and **Timer** in `ow_config.h`.  

---

## 🚀 Usage Example

### Initialization

```c
ow_handle_t ow;
ow_init_t ow_init_cfg = {
    .tim_handle = &htim1,
    .gpio = GPIOA,
    .pin = GPIO_PIN_1,
    .tim_cb = HAL_TIM_PeriodElapsedCallback
};

ow_init(&ow, &ow_init_cfg);
```

### Write & Read

For single-device setups:

```c
uint8_t data[] = {0x44}; // Example: Convert T command for DS18B20
ow_write(&ow, OW_CMD_SKIP_ROM, data, sizeof(data));
```

For multi-device setups:

```c
uint8_t data[9];
ow_read(&ow, device_index, 0xBE, sizeof(data)); // Read Scratchpad
```

---

## 🧰 API Overview

| Function | Description |
|----------|-------------|
| `ow_init()` | Initialize ONEW handle |
| `ow_callback()` | Timer callback (must be called in IRQ) |
| `ow_is_busy()` | Check if bus is busy |
| `ow_last_error()` | Get last error |
| `ow_update_rom_id()` | Update ROM ID(s) |
| `ow_write()` | Write command & data |
| `ow_read()` | Read from device |
| `ow_read_resp()` | Get response buffer |

---

## 🔧 Configuration (`ow_config.h`)

You can define maximum devices and buffer sizes:

```c
#define OW_MAX_DEVICE     4      // Number of supported devices
#define OW_MAX_DATA_LEN   32     // Max data length
```

---

## 📜 License

This project is licensed under the terms specified in the [LICENSE](./LICENSE) file.  

---

