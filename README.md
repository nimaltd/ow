# ⚡ Non-Blocking One-Wire (1-Wire) Library for STM32  

A lightweight and efficient **1-Wire protocol** library written in C for STM32 (HAL-based).  

Unlike blocking implementations, this library uses **asynchronous, non-blocking operation** driven by a single timer interrupt.  
It can run on **any GPIO pin** configured as open-drain, making it fully flexible and easy to integrate into your STM32 projects.  

It supports a wide range of Maxim/Dallas devices, including:  

- 🌡️ **DS18B20** – Temperature sensor  
- 🔋 **DS2431** – EEPROM  
- 🔐 **DS28E17** – I²C master bridge  
- 🔒 **DS1990A iButton** – Authentication token  
- ⚡ Any other standard 1-Wire device  

The library is designed for:  

- Projects where CPU blocking must be avoided  
- Applications that need **multi-device 1-Wire bus support**  
- Easy portability across different STM32 families (F0/F1/F3/F4/F7/G0/G4/H7, etc.)  

---

## ✨ Features  

- 🔹 Single or multiple device support (`OW_MAX_DEVICE`)  
- 🔹 Full STM32 HAL compatibility  
- 🔹 ROM ID operations (read, match, skip, search)  
- 🔹 Robust error handling (reset, bus, ROM, length)  
- 🔹 Non-blocking operation via timer callbacks  
- 🔹 Clean and modular API  

---

## ⚙️ Installation  

You can install in two ways:  

### 1. Copy files directly  
Add these files to your STM32 project:  
- `ow.h`  
- `ow.c`  
- `ow_config.h`  

### 2. STM32Cube Pack Installer (Recommended)  
Available in the official pack repo:  
👉 [STM32-PACK](https://github.com/nimaltd/STM32-PACK)  (Not Ready)

---

## 🔧 Configuration (`ow_config.h`)  

Defines library limits and timing values. Example:  

```c
#define OW_MAX_DEVICE     4      // Max number of devices
#define OW_MAX_DATA_LEN   32     // Max data length
#define OW_2_PINS         0      // Enable if using dual pins(TX/RX) for isolation 
```  

---

## 🛠 CubeMX Setup  

1. **GPIO Pin**  
   - Configure as **Output Open-Drain** (e.g., `PC8`).  
   - In dual pins mode, set TX pin as **Output Push-PULL** and set RX pin as ** INPUT ** (e.g., `PC8`, `PC7`). 

2. **Timer**  
   - Use **internal clock source**.  
   - Prescaler set for `1 µs` tick.  
     - Example: 170 MHz bus → Prescaler = `170 - 1`.  
   - Enable **Timer NVIC interrupt**.  
   - In **Project Manager → Advanced Settings**, enable **Register Callback** for the timer.
   - In **Project Manager → Code Generator**, enable **Generate Peripheral initialization as a pair ".c/.h" files per peripheral**.  

---

## 🚀 Quick Start  

### Include header  
```c
#include "ow.h"
```  

### Define a handle  
```c
ow_handle_t ds18;
```  

### Create a timer callback  
```c
void ds18_tim_cb(TIM_HandleTypeDef *htim)
{
    ow_callback(&ds18);
}
```  

### Optional Done Callback
```c
void ds18_done_cb(ow_err_t error)
{
}

```

### Initialize 1 pin mode in `main.c`  
```c
ow_init_t ow_init_struct;
ow_init_struct.tim_handle = &htim1;
ow_init_struct.gpio = GPIOC;
ow_init_struct.pin = GPIO_PIN_8;
ow_init_struct.tim_cb = ds18_tim_cb;
ow_init_struct.done_cb = ds18_done_cb;   // Optional: callback when transfer is done, or can use NULL

ow_init(&ds18, &ow_init_struct);
```  

Now the library is ready—use any `ow_*` functions.  

### Example: Reading temperature from DS18B20:
```c 
uint8_t data[16];
ow_update_rom_id(&ds18);
while (ow_is_busy(&ds18));
HAL_Delay(10);
ow_xfer_by_id(&ds18, 0, 0x44, NULL, 0, 0);
HAL_Delay(1000);
ow_xfer_by_id(&ds18, 0, 0xBE, NULL, 0, 9);
while (ow_is_busy(&ds18));
ow_read_resp(&ds18, data, 16);
```

## 🧰 API Overview  

| Function | Description |
|----------|-------------|
| `ow_init()` | Initialize one-wire handle |
| `ow_callback()` | Timer callback (must be called in IRQ) |
| `ow_crc()` | Calculate CRC |
| `ow_is_busy()` | Check if bus is busy |
| `ow_last_error()` | Get last error |
| `ow_update_rom_id()` | Detect and update connected ROM IDs |
| `ow_xfer()` | Write command + Read/Write data to/from the bus (no specific ROM ID) |
| `ow_xfer_by_id()` | Write command + Read/Write data to/from the bus (selected ROM ID) |
| `ow_devices()` | Get number of detected devices *(only if multi-device enabled)* |
| `ow_read_resp()` | Copy response buffer to user data |

---

## 💖 Support  

If you find this project useful, please **⭐ star** the repo and consider supporting!  

- [![GitHub](https://img.shields.io/badge/GitHub-Follow-black?style=for-the-badge&logo=github)](https://github.com/NimaLTD)  
- [![YouTube](https://img.shields.io/badge/YouTube-Subscribe-red?style=for-the-badge&logo=youtube)](https://youtube.com/@nimaltd)
- [![Instagram](https://img.shields.io/badge/Instagram-Follow-blue?style=for-the-badge&logo=instagram)](https://instagram.com/github.nimaltd)
- [![LinkedIn](https://img.shields.io/badge/LinkedIn-Connect-blue?style=for-the-badge&logo=linkedin)](https://linkedin.com/in/nimaltd)
- [![Email](https://img.shields.io/badge/Email-Contact-red?style=for-the-badge&logo=gmail)](mailto:nima.askari@gmail.com)
- [![Ko-fi](https://img.shields.io/badge/Ko--fi-Support-orange?style=for-the-badge&logo=ko-fi)](https://ko-fi.com/nimaltd)

---

## 📜 License  

Licensed under the terms in the [LICENSE](./LICENSE.TXT).  

---
