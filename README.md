# NV3007 142x428 dashboard

PlatformIO project for ESP32-C3 SuperMini and NV3007 SPI display.

## Wiring

| Display | ESP32-C3 GPIO |
|---|---:|
| GND | GND |
| VDD | 3V3 |
| SCL | GPIO5 |
| SDA | GPIO4 |
| RES | GPIO3 |
| DC | GPIO6 |
| CS | GPIO7 |
| BL | GPIO10 |

Use 3.3 V logic. The sketch uses the NV3007 initialization sequence from the working color-test sketch and renders a 428x142 landscape dashboard via a rotated framebuffer.

## Build

Open the repository in PlatformIO and build/upload environment `esp32-c3-devkitm-1`.
