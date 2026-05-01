# XR872 Drone Camera Reverse Engineering

Research and documentation for reverse engineering the camera module firmware from a Wi-Fi drone module based on the **Xradio XR872ET** SoC (ARM Cortex-M4F).

## Objective
The goal is to use the camera module as a standalone device without the proprietary drone smartphone app. Specifically, we aim to:
1. Identify the specific network port used for the video stream.
2. Uncover the exact hexadecimal "wake up" payload required to trigger the stream.
3. Locate hidden stream URLs (RTSP/MJPEG).

## Hardware Deep Dive: XR872ET SoC
The **Xradio XR872ET** is a high-performance, low-power wireless System-on-Chip (SoC) designed for IoT applications with multimedia requirements.

### **Core Specifications**
- **CPU**: ARM Cortex-M4F (with FPU and DSP instructions).
- **Clock Speed**: Up to **384 MHz**.
- **Memory**: 
  - **Internal SRAM**: 416 KB.
  - **Internal ROM**: 160 KB.
  - **External Support**: Supports QSPI Flash and OPI PSRAM.
- **Multimedia**:
  - **Hardware JPEG Encoder**: Essential for high-frame-rate video streaming.
  - **CSI Interface**: Used to interface with the GC0328 and other sensors.
- **Security**: Hardware crypto engine (AES/DES/SHA/CRC).

## Firmware Architecture
The `factory-firmwire.bin` file is a composite flash image using the Xradio **AWIH** (Allwinner/Xradio Image Header) format. It consists of multiple partitions:

### **Image Partitions (Flash Layout)**
| Flash Offset | Type | Section ID | Description |
|--------------|------|------------|-------------|
| `0x00000`    | `boot` | `0xa5ff5a00` | Primary Bootloader |
| `0x08000`    | `app`  | `0xa5fe5a01` | Application Metadata & RAM sections |
| `0x11C00`    | `app`  | `0xa5fd5a02` | Main Application Code / RAM Sections |
| `0x61800`    | `xip`  | `0xa5fd5a02` | Execute-In-Place (XIP) code partition |

### **Partition Analysis**
- **Bootloader (`0x00000`)**: Handles initial SoC setup and loads the application image.
- **Application (`0x11C00`)**: Contains the core logic, including the lwIP network stack and the "WiFiUFO" control logic.
- **XIP Section (`0x61800`)**: Code that runs directly from the SPI flash to save internal SRAM. This is where large functions (like video processing and protocol handling) are often stored.

## Reverse Engineering Insights
- **Target Address**: When analyzing the `app` section in Ghidra, use the load address specified in the header at `0x8000` (typically **`0x00201000`**) or the bootloader load address (**`0x00268000`**).
- **Strings Anchor**: The typo `sream start` is located in the middle of the binary, likely within the `app` partition. This confirms that the camera control logic is part of the high-level application code, not the bootloader.


## Tools Used
- `binwalk`: For entropy analysis and extraction.
- `ghidra`: For decompilation and static analysis.
- `hexdump` / `strings`: For initial binary autopsy.

---
*Disclaimer: This project is for educational and research purposes only.*
