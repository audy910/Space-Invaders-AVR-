# Space Invaders on ATmega1284

A real-time **Space Invaders** game developed in **C** using an **ATmega1284 microcontroller**. The game leverages a synchronous task scheduler for smooth gameplay and integrates display, input, and audio peripherals.

## Features

- **Real-time gameplay** using a synchronous task scheduler for smooth and responsive controls.
- **LCD graphics rendering** on a HiLetgo 1.44" SPI display with optimized SPI communication for efficient frame updates.
- **Joystick controls** for player movement and shooting.
- **2×18 LED display** to track score and remaining lives.
- **PWM-driven audio feedback** via a passive buzzer for dynamic sound effects when winning, losing, or shooting.

## Hardware Used

- ATmega1284 Microcontroller
- HiLetgo 1.44" SPI LCD Display
- Joystick module
- 2×18 LED Display
- Passive Buzzer

## Software

- Written in **C**
- Synchronous task scheduler for multitasking
- SPI communication for LCD rendering
- PWM audio generation for buzzer feedback

## Usage

1. Connect the microcontroller to the SPI LCD, joystick, LED display, and buzzer as per the wiring diagram.
2. Compile the C code using your preferred AVR toolchain.
3. Upload the program to the ATmega1284 microcontroller.
4. Use the joystick to move the player character and shoot enemies.
5. Track your score and remaining lives via the LED display.

## Notes

- Optimized SPI communication ensures smooth frame rendering without flicker.
- PWM audio is used to provide immersive sound effects during gameplay.
- Designed for embedded systems coursework and demonstration of real-time task scheduling in C.

## License

This project is for educational purposes and demonstration of embedded systems concepts.
