/********************************************************************************
 * MINT Header
 * Jason CJ Tay
 ********************************************************************************/

#ifndef __MINT_H__
#define __MINT_H__

#ifdef __cplusplus
 extern "C" {
#endif

// Uncomment the next line if running on 64-bit systems like RV64GC
//#define __64bit__

#define APP_UART_TXTIMEOUT 100
#define INBUF_MAX  256
#define EXBUF_SIZE 1024
#define LOOP_MAX   32

#define KEY_LISTDEFS 12
#define KEY_ENTER    13
#define KEY_CTRLP    16
#define KEY_DELETE   127

#define MINT_STACK_SIZE 32

#define ERR_NONE      0
#define ERR_OVERFLOW  1
#define ERR_UNDERFLOW 2
#define ERR_BAD_PARAM 3

/* Number mode */
#define NUMODE_DEC    0
#define NUMODE_HEX    1

#define RETURN_NONE      0
#define RETURN_TO_RAM    1
#define RETURN_TO_FLASH  2
#define RETURN_TO_EEPROM 3

typedef signed char INT8;
typedef unsigned char BYTE;   // uint8_t

#ifdef __64bit__
typedef long CELL_T;
typedef long* CELL_PTR;
#else
typedef int CELL_T;
typedef int* CELL_PTR;
#endif

#ifndef NULL
#define NULL 0
#endif

extern void mintInit(void);
extern void mintRun(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
