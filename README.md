# CMINT
An implementation of the Minimal Interpreted Language in portable C.
Original version for the Z80-powered TEC-1 Computer by [Ken Boak](https://github.com/monsonite), [John Hardy](https://github.com/jhlagado)
and [Craig Jones](https://github.com/crsjones). MINT is a human readable bytecode interpreter.
This version is known to run reasonably well on the Arduino UNO/Nano, 8051 (SDCC), STM32 (Keil & STM32Cube), ESP32, ESP32-C3, RP2040.

Refer to <https://github.com/orgMINT/MINT> to find the original project and to learn the language.

## Yet to be done
1. Arrays [] - entering balanced [] is already supported by the line editing routine, but not by the interpreter.
2. Break from loop.
3. Pin GPIO input/output \\> and \\<

## Differences from the original MINT
1. CMINT will abort input or execution with an error message, whereas original Z80 MINT often will continue.
2. If there aren't enough elements in the stack, original Z80 MINT will often retrieve 0, but CMINT will trigger a stack underflow error.
3. CMINT has a \\m bytecode that will immediately push the current milliseconds elapsed since bootup or since the last timer overflow.

## Porting Instructions
1. You need to take the mint.c and mint.h files (renaming extensions as needed for convenience), add it to the project of your platform/IDE of choice.
2. Then you need to implement the following functions in your own project:
- txChar()
- rxChar()
- available()
- getMillis()
