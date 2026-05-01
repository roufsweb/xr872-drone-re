# XR872 Drone Camera Reverse Engineering

Research and documentation for reverse engineering the camera module firmware from a Wi-Fi drone module based on the **Xradio XR872ET** SoC (ARM Cortex-M4F).

## Objective
The goal is to use the camera module as a standalone device without the proprietary drone smartphone app. Specifically, we aim to:
1. Identify the specific network port used for the video stream.
2. Uncover the exact hexadecimal "wake up" payload required to trigger the stream.
3. Locate hidden stream URLs (RTSP/MJPEG).

## Hardware Overview
- **SoC**: Xradio XR872ET (ARM Cortex-M4F, Little Endian)
- **Flash**: T25S40 SPI NOR (512KB)
- **OS**: FreeRTOS-based
- **Network Stack**: lwIP
- **Camera Sensors**: gc0328, gc2015, sp0A19 (referenced in drivers)
- **Hardware Encoder**: CSI_JPEG

## Current Findings

### Networking
- **SSID**: `WiFiUFO_`
- **Gateway**: `192.168.28.1`
- **IP Range**: `192.168.28.2` - `192.168.28.10`
- **Open Ports**: None found during standard scan (1-9999). Likely requires a magic packet to "wake up" the server.

### Strings & Commands
The binary contains specific strings used for camera control, including a notable typo ("sream"):
- `sream start` (Offset: `0xE76C` in firmware)
- `sream stop` (Offset: `0xE779`)
- `stream sw` (Offset: `0xE785`)

## Technical Analysis

### Firmware Header (`AWIH`)
The firmware uses the Allwinner/Xradio Image Header format.
- **Load Address**: `0x00268000` (Use this as the base address in Ghidra)
- **Entry Point**: `0x00268101`

### Reverse Engineering Strategy
1. **Tooling**: Use Ghidra with the `ARM:LE:32:Cortex (v7m)` language.
2. **Anchor**: Use the string `sream start` to locate the command dispatcher.
3. **Tracing**: Follow the cross-references from the command handler to identify the network task calling it.
4. **Port Discovery**: Look for the `lwip_bind` call preceding the main receive loop in the network task.

## Tools Used
- `binwalk`: For entropy analysis and extraction.
- `ghidra`: For decompilation and static analysis.
- `hexdump` / `strings`: For initial binary autopsy.

---
*Disclaimer: This project is for educational and research purposes only.*
