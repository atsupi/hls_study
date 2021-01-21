/******************************************************
 *    Filename:     fb_gfxaccel_inst.c 
 *     Purpose:     test app for graphics acceleration IP
 *  Target Plf:     ZYBO 
 *  Created on: 	2021/01/21
 *      Author: 	atsupi.com
 *     Version:		1.0
 ******************************************************/

//#define DEBUG

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "azplf_bsp.h"
#include "azplf_hal.h"
#include "azplf_util.h"

// Frame Buffer Memory Base Address
//#define MEM_BASE_ADDR		(DDR_BASE_ADDR + 0x01000000)

//#define READ_ADDRESS_BASE	MEM_BASE_ADDR
//#define WRITE_ADDRESS_BASE	MEM_BASE_ADDR

/* 
 * Data address
 * 
 * Read and write sub-frame use the same settings
 */
static u32 WriteFrameAddr[NUMBER_OF_FRAMES];
static u32 ResourceAddr; // resource buffer
static u32 CamFrameAddr; // dummy camera buffer

// driver instances
static VdmaInstance vdmaInst_0;
static VdmaInstance vdmaInst_1;  /* dummy for testing */
static LQ070outInstance lq070Inst;
static GfxaccelInstance gfxaccelInst;

static int fbActive;
static int fbBackgd;

/******************* Function Prototypes ************************************/

static int ReadSetup(VdmaInstance *inst);

void drawTrianglePolygons(void)
{
	pos points[] = {
		{ 100, 100 },
		{ 160, 100 },
		{ 130, 150 },
		{ 190, 150 },
		{ 160, 200 },
		{ 220, 200 },
		{ 190, 250 },
		{ 250, 250 },
		{ 220, 200 },
		{ 280, 200 },
		{ 250, 250 },
		{ 310, 250 },
		{ 280, 300 },
		{ 340, 300 },
		{ 310, 350 },
		{ 370, 350 },
		{ 340, 300 },
		{ 400, 300 },
		{ 370, 250 },
		{ 430, 250 },
		{ 400, 200 },
		{ 460, 200 },
		{ 430, 150 },
		{ 490, 150 },
		{ 460, 100 },
		{ 520, 100 },
	};
	int i;

	gfxaccel_draw_line(&gfxaccelInst, WriteFrameAddr[0], points[0].x, points[0].y, points[1].x, points[1].y, 0xffffffff);
	for (i = 0; i < sizeof(points)/sizeof(points[0]) - 2; i++)
	{
		gfxaccel_draw_line(&gfxaccelInst, WriteFrameAddr[0], points[i+1].x, points[i+1].y, points[i+2].x, points[i+2].y, 0xffffffff);
		gfxaccel_draw_line(&gfxaccelInst, WriteFrameAddr[0], points[i+2].x, points[i+2].y, points[i].x, points[i].y, 0x3ff00000);
	}
}

static void fillColorTiles(u32 baseAddr)
{
	int i, j;
	unsigned int col, data;

	for (i = 0; i < 480 / 16; i++) {
		for (j = 0; j < 800 / 16; j++) {
			col = ((i) << 2) | (j);
			data = RGB1(col >> 2, col >> 1, col);
			gfxaccel_fill_rect(&gfxaccelInst, baseAddr, j * 16, i * 16, j * 16 + 15, i * 16 + 15, data);
		}
	}
}

static void copyFramebufferToBitmap(u32 *dst, u32 *src, int width, int height, int stride)
{
	int x, y;
	u32 pix;
	u32 *ptr = src;
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pix = ptr[x];
			// Shift VDMA bitmap pixel data to RGB bitmap
			pix = ((pix & 0x3FC00000) >> 6) | ((pix & 0xFF000) >> 4) | ((pix & 0x3FC) >> 2);
			*dst++ = pix;
		}
		ptr += stride;
	}
}

static void saveFBtoBitmapFile(u32 baseAddr)
{
	Bitmap bmp;
	loadBitmapFile("base.bmp", &bmp);
	copyFramebufferToBitmap(bmp.data, (u32 *)vdma_get_frame_address(&vdmaInst_0, 0), DISP_WIDTH, DISP_HEIGHT, DISP_WIDTH);
	saveBitmapFile(&bmp, "result.bmp");
	free(bmp.data);
}

static int parse_argument(int argc, char *argv[])
{
	int mode = 0;
	int i;

	for (i = 0; i < argc; i++)
	{
		if (*argv[i] == '-' && *(argv[i]+1) == 's')
		{
			printf("  Shot mode enabled.\n");
			mode = 1;
			break;
		}
		else if (*argv[i] == '-' && *(argv[i]+1) == 'd')
		{
			printf("  Debug mode enabled.\n");
			mode = 2;
			break;
		}
	}

	return (mode);
}

int main(int argc, char *argv[])
{
	int i, j;
	int x, y;
	u32 size = FRAME_HORIZONTAL_LEN * FRAME_VERTICAL_LEN;
	int status;
	int mode;
	u32 ReadAddr;

	mode = parse_argument(argc, argv);

	WriteFrameAddr[0] = WRITE_ADDRESS_BASE;
	WriteFrameAddr[1] = WRITE_ADDRESS_BASE + frame_page;
	ResourceAddr      = WriteFrameAddr[1]  + frame_page;
	CamFrameAddr      = ResourceAddr + frame_page;

	fbActive = 0;
	fbBackgd = 1;

	printf("\r\n--- Entering main() --- \r\n");

	/* The information of the XAxiVdma_Config comes from hardware build.
	 * The user IP should pass this information to the AXI DMA core.
	 */
	printf("Step 0\n");
	vdma_init(&vdmaInst_0, VDMA_0_BASEADDRESS, WriteFrameAddr[0]);
	if (VDMA_INSTANCE_NUM > 1) {
//		vdma_init(&vdmaInst_1, VDMA_1_BASEADDRESS, CamFrameAddr);
	}

	printf("Step 1\n");
	/* Setup the read channel
	 */
	status = ReadSetup(&vdmaInst_0);
	if (status != PST_SUCCESS) {
		printf("Read channel setup failed %d\r\n", status);
		if(status == PST_VDMA_MISMATCH_ERROR)
			printf("DMA Mismatch Error\r\n");
		return PST_FAILURE;
	}
	printf("Step 2\n");

	// Start DMA engine to transfer
	status = vdma_start(&vdmaInst_0, VDMA_READ);
	if (status != PST_SUCCESS) {
		printf("Start read transfer failed %d\r\n", status);
		return PST_FAILURE;
	}

	status = vdma_start_parking(&vdmaInst_0, VDMA_READ, fbActive);
	if (status != PST_SUCCESS) {
		printf("Start Park failed\r\n");
		return PST_FAILURE;
	}
	printf("Test passed\r\n");

	lq070out_init(&lq070Inst, LQ070OUT_BASE_ADDR);
    lq070out_setmode(&lq070Inst, LQ070_PG_NONE);
    printf("Hello World\n\r");

    printf("Initialize gfxaccel instance\r\n");
	if (gfxaccel_init(&gfxaccelInst, GFXACCEL_BASE_ADDR) == PST_FAILURE) {
		return PST_FAILURE;
	}

    // Clear frame buffer
    printf("Clear frame buffer\r\n");
    gfxaccel_fill_rect(&gfxaccelInst, WriteFrameAddr[0], 0, 0, 799, 479, 0x0);

    // Setup Resource frame
    printf("Setup Resource frame buffer\r\n");
    fillColorTiles(ResourceAddr);

    printf("BitBlt Resource frame buffer to main frame buffer\r\n");
    for (i = 0; i < 480 / 16; i++)
    {
    	for (j = 0; j < 800 / 16; j++)
    	{
    		gfxaccel_bitblt(&gfxaccelInst, ResourceAddr, j * 16, (i /*% 15*/) * 16, 16, 16, WriteFrameAddr[0], j * 16, i * 16, GFXACCEL_BB_NONE);
    	}
    }

    // Invoke fill rectangle accelerator
    gfxaccel_fill_rect(&gfxaccelInst, WriteFrameAddr[0], 80, 80, 719, 399, RGB8(255, 255, 255));
    // Invoke fill rectangle accelerator
    gfxaccel_fill_rect(&gfxaccelInst, WriteFrameAddr[0],  83,  83, 716, 396, RGB8(0, 64, 255));
    // Invoke fill rectangle accelerator
    gfxaccel_fill_rect(&gfxaccelInst, WriteFrameAddr[0],  80, 240, 719, 241, RGB8(255, 255, 255));
    // Invoke fill rectangle accelerator
    gfxaccel_fill_rect(&gfxaccelInst, WriteFrameAddr[0], 480, 240, 481, 399, RGB8(255, 255, 255));

    // Invalidate frame buffer to reload updated data

    // Output results
	ReadAddr = vdma_get_frame_address(&vdmaInst_0, 0);
    printf("Read Frame address: 0x%x", ReadAddr);
    for (i = 0; i < 8; i++)
    {
    	printf("\r\nfb(%03d): ", i);
    	for (j = 0; j < 16; j++)
		{
    		printf("%02x ", FB(ReadAddr, ((i+76)*800+(j+76))) & 0xFF);
		}
    }
	printf("\r\n");
	saveFBtoBitmapFile(ReadAddr);

	drawTrianglePolygons();
	sleep(1); // 1sec wait

	x = 0;
	y = 0;
	printf("Loop starts.\r\n");
	while (1) {
		gfxaccel_bitblt(&gfxaccelInst, ResourceAddr, 0, 0, 800, 480, WriteFrameAddr[fbBackgd], 0, 0, GFXACCEL_BB_NONE);
		gfxaccel_fill_rect(&gfxaccelInst, WriteFrameAddr[fbBackgd], x * 32, y * 32, x * 32 + 63, y * 32 + 63, RGB8(255, 255, 255));
	    x += 1;
	    if (x == 24)
	    {
	    	x = 0;
	    	y += 1;
	    	if (y == 14) y = 0;
	    }
		fbActive ^= 1;
		fbBackgd ^= 1;
		status = vdma_start_parking(&vdmaInst_0, VDMA_READ, fbActive);
		if (status != PST_SUCCESS) {
			printf("Start Park failed\r\n");
			return PST_FAILURE;
		}
#ifdef DEBUG
	    printf("current frame = %d\r\n", fbActive);
#endif
		usleep(20000); // 20ms wait
	}
    printf("--- Exiting main() --- \r\n");

	return 0;
}

/*****************************************************************************
 *
 * This function sets up the read channel
 *
 *****************************************************************************/
static int ReadSetup(VdmaInstance *inst)
{
	int Index;
	u32 Addr;
	int status;

	printf("Step 2.1\n");
	status = vdma_config(inst, VDMA_READ);
	if (status != PST_SUCCESS) {
		printf("Read channel config failed %d\r\n", status);
		return PST_FAILURE;
	}

	printf("Step 2.2\n");
	// Initialize buffer addresses (physical addresses)
	Addr = READ_ADDRESS_BASE;
	for(Index = 0; Index < NUMBER_OF_FRAMES; Index++) {
		vdma_set_frame_address(inst, Index, Addr);
		Addr += frame_page;
	}

	printf("Step 2.3\n");
	// Set the buffer addresses for transfer in the DMA engine
	status = vdma_dma_setbuffer(inst, VDMA_READ);
	if (status != PST_SUCCESS) {
		printf("Read channel set buffer address failed %d\r\n", status);
		return PST_FAILURE;
	}

	printf("Step 2.4\n");
	return PST_SUCCESS;
}
