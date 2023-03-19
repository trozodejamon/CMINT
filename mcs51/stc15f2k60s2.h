/* STC15F2K60S2 Support
 * Need to double check registers!
 */

/* For the STC15F2K60S2 */
__sfr __at (0x8E) AUXR;
__sfr __at (0x9D) P1ASF;
__sfr __at (0xBC) ADC_CONTR;
__sfr __at (0xBD) ADC_RES;
__sfr __at (0xBE) ADC_RESL;
__sfr __at (0xD6) T2H;
__sfr __at (0xD7) T2L;

__sfr __at (0xA1) BUS_SPEED;
__sbit __at (0xC8) P5_0;
__sbit __at (0xC9) P5_1;
__sbit __at (0xCA) P5_2;
__sbit __at (0xCB) P5_3;
__sbit __at (0xCC) P5_4;
__sbit __at (0xCD) P5_5;

// Other ADC related defines
#define ADC_POWER	0x80
#define ADC_FLAG	0x10		// ADC conversion complete flat
#define ADC_START	0x08		// ADC start control bit
#define ADC_SPEEDLL	0x00		// 540 clocks
#define ADC_SPEEDL	0x20		// 360 clocks
#define ADC_SPEEDH	0x40		// 180 clocks
#define ADC_SPEEDHH 0x60		// 90 clocks

