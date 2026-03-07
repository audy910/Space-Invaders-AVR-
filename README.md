# Space Invaders on ATmega1284

A real-time **Space Invaders** game built in **C/C++** for the **ATmega1284**. The project uses a cooperative (synchronous) task scheduler to keep gameplay responsive while coordinating graphics, input, score/lives output, and sound effects.

## Overview

This project demonstrates how to build a small game loop on resource-constrained hardware by splitting work across periodic tasks:

- Read joystick input
- Update player/projectile/enemy state
- Render frames to the SPI LCD
- Update score/lives output
- Drive PWM sound effects

---

## Hardware Architecture

```mermaid
flowchart LR
    MCU[ATmega1284 MCU]
    LCD[HiLetgo 1.44 in <br> ST7735 SPI LCD]
    JOY[Joystick Module <br> ADC X/Y + Button]
    LED[2x16 LED Display <br> Score + Lives]
    BUZ[Passive Buzzer <br> PWM Output]

    MCU -- SPI --> LCD
    JOY -- Analog + Digital --> MCU
    MCU -- GPIO --> LED
    MCU -- Timer/PWM --> BUZ
```

### Main Peripherals

- **Display:** ST7735-based 1.44" SPI LCD for rendering gameplay.
- **Input:** Joystick for left/right movement and up for firing. Button for hard restart.
- **Output:** LED display for score and lives.
- **Audio:** Passive buzzer driven with PWM patterns.

### Wiring Schematic

---
#### 16 x 2 LCD Display 
The LCD is wired for a standard 4-bit data interface.
| Arduino Pin | LCD Pin Label | Function | Connection Type |
| :--- | :--- | :--- | :--- |
| GND | VSS | Ground | Ground Rail  |
| 5V | VDD | Logic Power | Power Rail  |
| - | VO | Contrast | Potentiometer (Wiper)  |
| D2 | RS | Register Select | Digital Signal  |
| GND | RW | Read/Write | Ground Rail  |
| D3 | E | Enable | Digital Signal  |
| D4 | D4 | Data Bit 4 | Digital Signal  |
| D5 | D5 | Data Bit 5 | Digital Signal  |
| D6 | D6 | Data Bit 6 | Digital Signal  |
| D7 | D7 | Data Bit 7 | Digital Signal  |
| 5V | A | Display Power | Power Rail |
| GND | K | Ground | Ground Rail |


#### HiLetgo Module (SPI LCD display)

This module utilizes the high-speed SPI pins for game rendering.

| Arduino Pin | Module Pin Label | Function |
| :--- | :--- | :--- |
| 5V | LED | LED | Backlight Power  |
| D13 | SCK | Serial Clock  |
| D11 | SDA | Serial Data (MOSI)  |
| D8 | AO | Data/Command Select  |
| D10 | RESET | Module Reset  |
| D12 | CS | Chip Select  |
| GND | GND | Ground Rail |
| 5V | VCC | Logic Power |

#### Joystick 
| Arduino Pin | Module Pin Label | Function |
| :--- | :--- | :--- |
| GND | GND | Ground |
| 5V | 5V | Power |
| A1 | VRX | Serial Data (MOSI)  |
| A2 | VRY | Data/Command Select  |
| A0 | SW | Module Reset  |

#### Other Components 
| Arduino Pin | Component |
| :--- | :--- | 
| A5 | Button |
| D9 | Buzzer | 



## Software / Task Flow

The program runs as a scheduled set of finite-state-machine-style tasks.

```mermaid
flowchart TD
    Start([Power On / Reset]) --> Init[Initialize peripherals<br>SPI, ADC, Timers, PWM, LCD]
    Init --> Scheduler[Start synchronous scheduler]

    Scheduler --> T1[Task: Read Input]
    Scheduler --> T2[Task: Update Game State]
    Scheduler --> T3[Task: Collision & Rules]
    Scheduler --> T4[Task: Render Frame]
    Scheduler --> T5[Task: Update Score/Lives]
    Scheduler --> T6[Task: Audio Control]

    T1 --> T2
    T2 --> T3
    T3 --> T4
    T4 --> T5
    T5 --> T6
    T6 --> Scheduler
```

### Game State Flow

```mermaid
stateDiagram-v2
    [*] --> Boot
    Boot --> Menu
    Menu --> Playing: Start input
    Playing --> Playing: Move / Shoot / Spawn / Collide
    Playing --> Win: All enemies cleared
    Playing --> Lose: Lives == 0
    Win --> Menu: Restart
    Lose --> Menu: Restart
```

---

## Project Structure

```text
.
├── src/
│   └── arein015_main.cpp      # Main game loop, scheduler, gameplay logic
├── include/
│   ├── st7735.h               # LCD driver interface
│   ├── spiAVR.h               # SPI utilities
│   ├── timerISR.h             # Timer/scheduler timing support
│   ├── analogHelper.h         # ADC / joystick helpers
│   ├── LCD.h                  # LCD helper utilities
│   └── serialATmega.h         # Serial debug support
├── platformio.ini             # Build configuration
└── README.md
```

---

## Build and Upload

### Option 1: PlatformIO (recommended)

1. Install [PlatformIO](https://platformio.org/).
2. Connect your ATmega1284 programming setup.
3. Build:
   ```bash
   pio run
   ```
4. Upload:
   ```bash
   pio run -t upload
   ```

### Option 2: AVR toolchain

- Compile with your AVR-GCC setup and flash using your preferred uploader.

---

## How to Play

1. Power on the system.
2. Use the joystick to move the player ship.
3. Trigger shoot input to fire projectiles.
4. Eliminate enemies while avoiding hits.
5. Watch score/lives on the LED display.
6. Listen for event-based buzzer feedback (shoot, win, lose).

---

## Why This Project Is Useful

- Demonstrates **real-time embedded scheduling**.
- Shows practical **SPI graphics** on constrained MCUs.
- Integrates mixed I/O: **ADC, GPIO, PWM, timers, and display control**.
- Good reference for **embedded game architecture** and classroom demos.

---

## License

This project is intended for educational and demonstration use.
