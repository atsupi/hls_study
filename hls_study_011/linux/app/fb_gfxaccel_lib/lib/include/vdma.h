/******************************************************
 *    Filename:     vdma.h 
 *     Purpose:     vdma controller driver 
 *  Created on: 	2015/04/05 
 * Modified on: 	2021/01/18 
 *      Author: 	atsupi.com 
 *     Version:		1.10 
 ******************************************************/

#ifndef VDMA_H_
#define VDMA_H_

#define DDR_BASE_ADDR				0x10000000
#define DDR_HIGH_ADDR				0x1F000000
#define MEM_BASE_ADDR				(DDR_BASE_ADDR + 0x01000000)
#define MEM_HIGH_ADDR				DDR_HIGH_ADDR
#define MEM_SPACE					(MEM_HIGH_ADDR - MEM_BASE_ADDR)
#define READ_ADDRESS_BASE			MEM_BASE_ADDR
#define WRITE_ADDRESS_BASE			MEM_BASE_ADDR

#define FRAME_HORIZONTAL_LEN		(DISP_WIDTH * 4) /* each pixel 4 bytes */
#define FRAME_VERTICAL_LEN			DISP_HEIGHT

#define SUBFRAME_HORIZONTAL_SIZE	(DISP_WIDTH * 4) /* each pixel 4 bytes */
#define SUBFRAME_VERTICAL_SIZE		DISP_HEIGHT

#define NUMBER_OF_READ_FRAMES		3
#define NUMBER_OF_WRITE_FRAMES		3
#define DELAY_TIMER_COUNTER			0

#define VDMA_READ					0
#define VDMA_WRITE					1

// AXI VDMA registers
#define VDMA_BASEADDRESS			XPAR_AXI_VDMA_0_BASEADDR
#define VDMA_BASEADDRESS_END		(VDMA_BASEADDRESS + 0xFF)
#define MM2S_VDMACR					(0x00)
#define MM2S_VDMASR					(0x04)
#define VDMA_PARKPTR				(0x28)
#define VDMA_VERSION				(0x2C)
#define S2MM_VDMACR					(0x30)
#define S2MM_VDMASR					(0x34)
#define MM2S_VSIZE					(0x50)
#define MM2S_HSIZE					(0x54)
#define MM2S_FRMDLY_STRIDE			(0x58)
#define MM2S_START_ADDRESS			(0x5C)
#define S2MM_VSIZE					(0xA0)
#define S2MM_HSIZE					(0xA4)
#define S2MM_FRMDLY_STRIDE			(0xA8)
#define S2MM_START_ADDRESS			(0xAC)

extern u32 frame_page;

extern void vdma_init(void);
extern void vdma_deinit(void);
extern int vdma_config(int mode);
extern int vdma_start(int mode);
extern int vdma_stop(int mode);
extern int vdma_dma_setbuffer(int mode);
extern void vdma_set_frame_address(int index, u32 addr);
extern u32 vdma_get_frame_address(int fbnum);
extern int vdma_start_parking(int mode, int fbnum);
extern int vdma_stop_parking(int mode);

extern u32 vdma_read_reg(u32 adr, u32 offset);
extern void vdma_write_reg(u32 adr, u32 offset, u32 value);

#endif /* VDMA_H_ */
