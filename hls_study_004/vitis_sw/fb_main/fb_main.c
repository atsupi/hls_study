/******************************************************
 *  Filename: main.c 
 *  Created on: 	2020/09/28
 *      Author: 	atsupi
 *     Version:		0.1
 ******************************************************/
#include <stdio.h>
#include "xil_printf.h"
#include "xparameters.h"
#include "xil_cache.h"
#define DDR_BASE_ADDR		XPAR_PS7_DDR_0_S_AXI_BASEADDR
#define DDR_HIGH_ADDR		XPAR_PS7_DDR_0_S_AXI_HIGHADDR

#define MEM_BASE_ADDR		(DDR_BASE_ADDR + 0x01000000)
#define MEM_HIGH_ADDR		DDR_HIGH_ADDR
#define MEM_SPACE			(MEM_HIGH_ADDR - MEM_BASE_ADDR)

#define READ_ADDRESS_BASE	MEM_BASE_ADDR
#define WRITE_ADDRESS_BASE	MEM_BASE_ADDR

#define FRAME_HORIZONTAL_LEN  800*4   /* 1920 pixels, each pixel 4 bytes */
#define FRAME_VERTICAL_LEN    480    /* 1080 pixels */

#define SUBFRAME_START_OFFSET    0
#define SUBFRAME_HORIZONTAL_SIZE 800*4
#define SUBFRAME_VERTICAL_SIZE   480
#define FB(addr, offset)		(*(volatile unsigned int *)((unsigned int *)(addr)+(unsigned int)(offset)))
#define RGB1(r, g, b)  (((((r) & 1) * 0x3ff << 20) | ((g) & 1) * 0x3ff << 10) | (((b) & 1) * 0x3ff))
#define RGB8(r, g, b)  ((((r) & 0xff) << 22) | (((g) & 0xff) << 12) | (((b) & 0xff) << 2))
#define RGB10(r, g, b) ((((r) & 0x3ff) << 20) | (((g) & 0x3ff) << 10) | ((b) & 0x3ff))
/* Data address
 *
 * Read and write sub-frame use the same settings
 */
static u32 ReadFrameAddr;
static u32 WriteFrameAddr;

static u32 BlockStartOffset;
static u32 BlockHoriz;
static u32 BlockVert;
int main()
{
	int Status;

	ReadFrameAddr = READ_ADDRESS_BASE;
	WriteFrameAddr = WRITE_ADDRESS_BASE;
	BlockStartOffset = SUBFRAME_START_OFFSET;
	BlockHoriz = SUBFRAME_HORIZONTAL_SIZE;
	BlockVert = SUBFRAME_VERTICAL_SIZE;

	xil_printf("\r\n--- Entering main() --- \r\n");
    print("Hello World\n\r");

	u32 Addr = READ_ADDRESS_BASE + BlockStartOffset;
	u32 Sw;
	int mode = 0;
	int i;

	xil_printf("--- Exiting main() --- \r\n");

	return XST_SUCCESS;
}
