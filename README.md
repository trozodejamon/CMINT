# CMINT
An implementation of the Minimal Interpreted Language in portable C.
Original version for the Z80-powered TEC-1 Computer by [Ken Boak](https://github.com/monsonite), [John Hardy](https://github.com/jhlagado)
and [Craig Jones](https://github.com/crsjones). MINT is a human readable bytecode interpreter.
This version is known to run reasonably well on the Arduino UNO/Nano, 8051 (SDCC), STM32 (Keil & STM32Cube), ESP32, ESP32-C3, RP2040.

Refer to <https://github.com/orgMINT/MINT> to find the original project and to learn the language.

The files needed to build the project with SDCC on an STC 8051 or similar can be found in the "mcs51" subfolder.

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

## CMINT Benchmarks
Benchmarks, because of course, we need to know. These benchmarks are of what has become a typical "million empty loop" test. Basically, it looks like this:

> 1000(1000())

But to this, we will add some special words available currently only in CMINT, the ```\m``` word, which pushes the number of milliseconds elapsed since bootup
(or the last rollover) onto the data stack, so it now looks like this:

>\m1000(1000())\m$-.

For the uninitiated, this means:
1. Push milliseconds onto top of stack (TOS).
2. Run 1000 loops of 1000 loops. (Why do it as nested loops? 1. To test nested loops and 2. So that the same code runs the same on 8-bit and 32-bit systems, which use different integer word lengths).
3. Push milliseconds onto TOS (so now we have before and after milliseconds).
4. Swap the two stack values so we now have them ordered as [after, before] with '$'.
5. Subtract before from after using '-'.
6. Pop the value off the stack and dispay the value on the terminal with '.'.

| Device | Core | MHz | Time | Clocks Per Iteration | Iterations/s | Dev Tool |
| ------ | ---- | --- | ---- | -------------------- | ------------ | -------- |
| Sipeed Maix Bit 2 | RISC-V RV64GC | 400 | 0.1557 | 62 |  6,422,607 | Arduino |
| ESP32-C3 | RISC-V RV32I | 160 | 0.392 | 63 |  2,551,020 | Arduino |
| ESP32 | Tensilica Xtensa LX7 | 240 | 0.4 | 96 |  2,500,000 | Arduino |
| RP2040 | ARM Cortex M0+ | 125 | 0.612 | 76 |  1,633,986 | Arduino |
| STM32F401 | ARM Cortex M4 | 84 | 1.007 | 85 |  993,048 | STM32CubeIDE |
| STM32F042 | ARM Cortex M0+ | 48 | 2.137 | 102 |  467,945 | Keil uVision 5 |
| ATmega328P | AVR | 16 | 5.116 | 82 |  195,465 | Arduino |
| STC15F2K60S2 | 8052 | 11.0572 | 32.4 | 143 |  30,864 | SDCC |
