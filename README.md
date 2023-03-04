# CMINT
An implementation of the Minimal Interpreted Language in portable C.
Original version for Z80 TEC-1 Computer by [Ken Boak](https://github.com/monsonite), [John Hardy](https://github.com/jhlagado)
and [Craig Jones](https://github.com/crsjones). MINT is a human readable bytecode interpreter.
This version is known to run reasonably well on the Arduino UNO/Nano, 8051 (SDCC), STM32 (Keil & STM32Cube), ESP32, ESP32-C3, RP2040.

Refer to <https://github.com/orgMINT/MINT> to find the original project and to learn the language.

## Porting Instructions
1. You need to take the mint.c and mint.h files (renaming extensions as needed for convenience), add it to the project of your platform/IDE of choice.
2. Then you need to implement the following functions in your own project:
- txChar()
- rxChar()
- available()
- getMillis()
