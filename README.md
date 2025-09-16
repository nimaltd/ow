# Non-Blocking 1-Wire Library for STM32  
---  
## Please Don’t Forget to **⭐ Star** this repo!<br>Support 💖 me by donating or following on social networks.<br>Your support keeps this project alive! ✨<br>

-  Author:     Nima Askari  
-  Github:     https://www.github.com/NimaLTD
-  Youtube:    https://www.youtube.com/@nimaltd  
-  LinkedIn:   https://www.linkedin.com/in/nimaltd  
-  Instagram:  https://instagram.com/github.NimaLTD  
---

A lightweight and efficient **One Wire (1-Wire)** protocol library written in C for STM32 (HAL-based).  
It provides full support for 1-Wire devices such as:  

- 🌡️ **DS18B20** (temperature sensor)  
- 🔋 **DS2431** (EEPROM)  
- 🔐 **DS28E17** (I²C master bridge)  
- 🔒 **DS1990A iButton** (authentication)  
- ⚡ Any other standard Maxim/Dallas 1-Wire device  

The library includes multi-device support, asynchronous handling, and easy integration with STM32 projects.  

---

## ✨ Features

- 🔹 Supports single or multiple 1-Wire devices  
- 🔹 Fully compatible with STM32 HAL
- 🔹 ROM ID management (read, match, skip, search)  
- 🔹 Error handling (bus, reset, ROM ID, length)  
- 🔹 Non-blocking operation using timer callbacks  
- 🔹 Simple and modular API  

---

## ⚙️ Installation

You can install the library in two ways:

### 1. Copy files directly  
Copy the following files into your STM32 project:  
- `ow.h`  
- `ow.c`  
- `ow_config.h`  

### 2. Use STM32Cube Pack Installer (Recommended)  
Install from the official pack repository:  
👉 [STM32-PACK](https://github.com/nimaltd/STM32-PACK)  

---

## 🔧 Configuration (`ow_config.h`)

This file is used to configure:  
- Maximum number of supported devices (`OW_MAX_DEVICE`)  
- Timing definitions  

For example:  

```c
#define OW_MAX_DEVICE     4      // Number of supported devices
#define OW_MAX_DATA_LEN   32     // Max data length
```

---

## 🛠 CubeMX Setup

1. **GPIO Pin**  
   - Configure a pin as **Output Open-Drain** (e.g., `PC8`).  

2. **Timer**  
   - Enable a timer with **internal clock source**.  
   - Set **Prescaler** to reach `1 µs` tick.  
     - Example: for `170 MHz` bus → Prescaler = `170 - 1`.  
   - Enable **Timer NVIC interrupt**.  
   - In **Project Manager → Advanced Settings**, enable **Register Callback** for the timer.  

---

## 🚀 Usage Example

### Include header
```c
#include "ow.h"
```

### Define a handle
```c
ow_handle_t ds18;
```

### Create a custom callback
```c
void ds18_tim_cb(TIM_HandleTypeDef *htim)
{
    ow_callback(&ds18);
}
```

### Initialize in `main.c`
```c
ow_init_t ow_init_struct;
ow_init_struct.tim_handle = &htim1;
ow_init_struct.gpio = GPIOC;
ow_init_struct.pin = GPIO_PIN_8;
ow_init_struct.tim_cb = ds18_tim_cb;

ow_init(&ds18 , &ow_init_struct);
```

Now the library is ready and you can use all `ow_*` functions.  

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

## 📜 License

This project is licensed under the terms specified in the [LICENSE](./LICENSE.TXT) file.  

---
