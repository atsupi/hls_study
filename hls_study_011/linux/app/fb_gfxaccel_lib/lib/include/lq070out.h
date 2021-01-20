/******************************************************
 *    Filename:     lq070out.h 
 *     Purpose:     LQ070 LCD display driver
 *  Created on: 	2021/01/20
 * Modified on:
 *      Author: 	atsupi.com 
 *     Version:		1.00
 ******************************************************/

#ifndef LQ070OUT_H_
#define LQ070OUT_H_

// definitions for LQ070 control IP
#define LQ070OUT_BASE_ADDR		XPAR_AXI_LQ070_OUT_0_BASEADDR
#define LQ070OUT_MODE_OFFSET	0x00
#define   LQ070_PG_NONE		  	0x00		// Pass-through
#define   LQ070_PG_CBAR		  	0x01		// Color Bar pattern
#define   LQ070_PG_LAMP		  	0x02		// Lamp pattern (64 step)
#define   LQ070_PG_HTCH		  	0x03		// Hatch pattern

// external functions
extern void lq070out_init(void);
extern void lq070out_write_reg(u32 adr, u32 offset, u32 value);
extern void lq070out_setmode(int mode);


#endif // LQ070OUT_H_
