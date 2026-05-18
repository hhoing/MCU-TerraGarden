# TerraGarden

A smart terrarium controller built on the [Seeed Wio Terminal](https://www.seeedstudio.com/Wio-Terminal-p-4509.html).  
It monitors environmental conditions and lets you control misting and background music directly from the device — no cloud connection required.

## Circuit Diagram

![Circuit Diagram](assets/circuit_diagram.png)

## Features

- **Environmental monitoring** — reads temperature and humidity from a BME280 sensor with color-coded status (blue = cold/dry, green = normal, red = hot/humid)
- **Misting control** — drives an ultrasonic atomizer at 3 PWM power levels via the 5-way joystick
- **Background music** — plays ASMR tracks through a DFRobot DFPlayer Mini; supports track selection and volume control
- **Multi-screen LVGL UI** — navigate between Sensor, Water Control, and Sound Control screens using the onboard buttons and joystick

## Hardware

| Component | Interface | Notes |
|-----------|-----------|-------|
| Seeed Wio Terminal | — | Main MCU + TFT display |
| BME280 | I2C (SDA/SCL) | Temperature & humidity sensor |
| Ultrasonic atomizer | PWM (A0) | 3 power levels via `analogWrite` |
| DFRobot DFPlayer Mini | SoftwareSerial (TX=7, RX=8) | MP3 player module |
| MicroSD card | — | Loaded into DFPlayer with audio tracks |
| Buttons (×3) | GPIO INPUT_PULLUP | Screen navigation |
| 5-way joystick | GPIO INPUT_PULLUP | Parameter control |

## Communication Protocols

```
[Wio Terminal]
     │
     ├─ I2C (SDA/SCL) ────► BME280
     │   • Address: 0x76 (default)
     │   • Reads: temperature, humidity
     │   • Library: Seeed_BME280
     │
     ├─ PWM (A0) ─────────► Ultrasonic atomizer
     │   • analogWrite: 0 / 100 / 180 / 255
     │   • Level 0 = OFF, Level 3 = full power
     │
     ├─ SoftwareSerial ───► DFRobot DFPlayer Mini
     │   • TX pin: 7,  RX pin: 8
     │   • Baud rate: 9600
     │   • Commands: play(n), pause(), volume(n)
     │   • Protocol: DFPlayer Mini serial command set
     │
     └─ GPIO (INPUT_PULLUP)
         • WIO_KEY_A / B / C  — screen navigation
         • WIO_5S_UP/DOWN/LEFT/RIGHT/PRESS — parameter control
```

### DFPlayer Mini Serial Protocol

Commands are sent as 10-byte frames over UART at 9600 baud:

```
[0x7E] [0xFF] [0x06] [CMD] [0x00] [param_hi] [param_lo] [checksum_hi] [checksum_lo] [0xEF]
```

| Command | Byte (CMD) | Description |
|---------|-----------|-------------|
| Play track n | 0x03 | Start playing file n |
| Pause | 0x0E | Pause playback |
| Set volume | 0x06 | Volume 0–30 |

### I2C — BME280

| Signal | Pin |
|--------|-----|
| SDA | Grove SDA (D4) |
| SCL | Grove SCL (D5) |
| Address | 0x76 |

## Controls

| Input | Screen | Action |
|-------|--------|--------|
| Button A | Any | Cycle to next screen |
| Button B | Any | Jump to Water Control |
| Button C | Any | Jump to Sensor screen |
| Joystick UP/DOWN | Water Control | Increase / decrease mist level (0–3) |
| Joystick PRESS | Water Control | Toggle misting ON/OFF |
| Joystick UP/DOWN | Sound Control | Increase / decrease volume |
| Joystick LEFT/RIGHT | Sound Control | Previous / next track |
| Joystick PRESS | Sound Control | Play / pause |

## Temperature & Humidity Thresholds

| Color | Temperature | Humidity |
|-------|-------------|----------|
| Blue (too low / too dry) | < 18 °C | < 65 % |
| Green (normal) | 18–25 °C | 65–90 % |
| Red (too high / too humid) | > 25 °C | > 90 % |

## Software Dependencies (Arduino IDE)

Install these libraries via the Arduino Library Manager or manually:

- [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI)
- [LVGL](https://github.com/lvgl/lvgl) (v7.x)
- [Seeed_BME280](https://github.com/Seeed-Studio/Grove_BME280)
- [DFRobotDFPlayerMini](https://github.com/DFRobot/DFRobotDFPlayerMini)

## Getting Started

1. Clone this repository and open `TerraGarden.ino` in the Arduino IDE.
2. Install the libraries listed above.
3. Select **Seeed Wio Terminal** as the board.
4. Place audio files on the DFPlayer Mini's microSD card (tracks 1–3 in the root directory).
5. Upload and power on.

## Project Structure

```
TerraGarden/
├── TerraGarden.ino   # Main sketch
├── assets/           # Screenshots
├── .gitignore
└── README.md
```


