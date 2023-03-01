/********************************************************************************
 * MINT for Arduino
 * Tested working on:
 * - ESP32 (Tensilica): 0.4s
 * - ESP32-C3 (RISC-V): 0.392s
 * - RP2040 (Pi Pico): 0.612s @ 125MHz
 * - Wio Terminal (ATSAMD51): TBD, but it was really fast
 * - Arduino Nano: 5.4s
 * - Kendryte K210: 0.1557s
 ********************************************************************************/
#include "mint.h"
#ifdef __64bit__
#include <time.h>
#endif

int available(void)
{
  return Serial.available();
}

void txChar(char ch)
{
  Serial.print(ch);
}

BYTE rxChar(void)
{
  return Serial.read();
}

// Only needed on Kendryte K210 else return millis()
CELL_T getMillis(void)
{
#ifdef __64bit__
  return clock();
#else
  return millis();
#endif
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  mintInit();
}

void loop() {
  // put your main code here, to run repeatedly:
  mintRun();
}
