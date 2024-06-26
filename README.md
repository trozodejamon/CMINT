# CMINT
An implementation of the Minimal Interpreted Language in portable C.
Original version for the Z80-powered TEC-1 Computer by [Ken Boak](https://github.com/monsonite), [John Hardy](https://github.com/jhlagado)
and [Craig Jones](https://github.com/crsjones). MINT is a human readable bytecode interpreter.
This version is known to run reasonably well on the Arduino UNO/Nano, 8051 (SDCC), STM32 (Keil & STM32Cube), ESP32, ESP32-C3, RP2040, and dsPIC33CK.

Refer to <https://github.com/orgMINT/MINT> to find the original project and to learn the language.

The files needed to build the project with SDCC on an STC 8051 or similar can be found in the "mcs51" subfolder.

## Yet to be done
1. Arrays [] - entering balanced [] is already supported by the line editing routine, but not by the interpreter.
2. Break from loop.
3. I would like to add some basic support for fixed-point math.

## Differences from the original MINT
1. CMINT will abort input or execution with an error message, whereas original Z80 MINT often will continue.
2. If there aren't enough elements in the stack, original Z80 MINT will often retrieve 0, but CMINT will trigger a stack underflow error.
3. CMINT has a \\m bytecode that will immediately push the current milliseconds elapsed since bootup or since the last timer overflow.
4. CMINT uses sandboxed, pre-allocated spaces for the built-in variables and the user defined dictionary of words. If you clear a definition, fragmentation will occur that can only be fixed with a reset. See mint.h for the defines that indicate the maximum input buffer and execution buffer sizes. The execution buffer (execBuf) is where user defined words are stored).

## Porting Instructions
1. You need to take the mint.c and mint.h files (renaming extensions as needed for convenience), add it to the project of your platform/IDE of choice.
2. Then you need to implement the following functions in your own project. Note prior to the 1st April 2024 updates, I unwittingly used function names that already existed in the Arduino Core and I was actually redefining them. Concerned about this, I have renamed these external functions so there is no overlap:

| Old Version | New Version |
| ----------- | ----------- |
| txChar() | txChar() |
| rxChar() | rxChar() |
| available() | rxAvail() |
| getMillis() | getMillis() |
| getPin() | getIOPin() |
| setPin() | setIOPin() |

In addition, the following new function has been defined to be implemented in the main.c:
- setIOPinType() - this is essentially equivalent to Arduino's pinMode() function.

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
| STM32H743 | ARM Cortex M7 | 480 | 0.067 | 12.86 |  14,925,373 | Arduino |
| Sipeed Maix Bit 2 | RISC-V RV64GC | 400 | 0.1557 | 62 |  6,422,607 | Arduino |
| ESP32-C3 | RISC-V RV32I | 160 | 0.392 | 63 |  2,551,020 | Arduino |
| ESP32 | Tensilica Xtensa LX7 | 240 | 0.4 | 96 |  2,500,000 | Arduino |
| dsPIC33CK | dsPIC | 100 | 0.563 | 56 | 1,776,199 | MPLAB X |
| RP2040 | ARM Cortex M0+ | 125 | 0.612 | 76 |  1,633,986 | Arduino |
| STM32F401 | ARM Cortex M4 | 84 | 1.007 | 85 |  993,048 | STM32CubeIDE |
| STM32F042 | ARM Cortex M0+ | 48 | 2.137 | 102 |  467,945 | Keil uVision 5 |
| ATmega328P | AVR | 16 | 5.116 | 82 |  195,465 | Arduino |
| STC15F2K60S2 | 8052 | 11.0572 | 32.4 | 143 |  30,864 | SDCC |

**Notes**
1. Compiler optimisations. The STM32F042 test were done at -O1 and -O3 optimisations. At -O3, the benchmark improved slightly to 2.033s.
2. To me, the standouts in these tests are the AVR, dsPIC and RISC-V. The ARM chips are all running in 32-bits operating on 32-bit values. The AVR is running in 8-bit working on 16-bit values. It's impressive how good its Iterations/MHz score is.
3. For those who contend that empty loops don't make for a good benchmark, I have since tried CMINT with a linear AND recursive version of the Fibonacci sequence generator, and while they do turn in a very slightly higher words per second execution (with linear being faster than recursive) than the empty loops, the difference wasn't big enough for me to say that the empty loops have no value...
4. For some reason, the RP2040 is 34% faster than the STM32F0, MHz-for-MHz, despite the fact that they both have ARM Cortex-M0+ cores.
5. STM32H743 result contributed by Ken Boak <https://github.com/monsonite>.

## Known Issues
1. The printDec() routine actually only displays +/- values within the range of +/- 32,767 on 8-bit systems using a 16-bit int as its cell size and 
+/- 2,147,483,647 on 32-bit systems. I.e., it can't display -32,768 or -2,147,483,648 on those respective systems.
