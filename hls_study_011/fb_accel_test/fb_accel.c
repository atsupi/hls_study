/******************************************************
 *    Filename:     fb_accel.c 
 *     Purpose:     test app for graphics acceleration IP
 *  Target Plf:     ZYBO 
 *  Created on: 	2021/01/14
 *      Author: 	atsupi.com
 *     Version:		1.10
 ******************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "xil_printf.h"
#include "xparameters.h"
#include "xaxivdma.h"
#include "xil_cache.h"
#include "xgfxaccel.h"
/*
 * Device related constants. These need to defined as per the HW system.
 */
#define DMA_DEVICE_ID		XPAR_AXIVDMA_0_DEVICE_ID

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

/* Number of frames to work on, this is to set the frame count threshold
 *
 * We multiply 15 to the num stores is to increase the intervals between
 * interrupts. If you are using fsync, then it is not necessary.
 */
#define NUMBER_OF_READ_FRAMES		1
#define NUMBER_OF_WRITE_FRAMES		1
#define DELAY_TIMER_COUNTER			10

// Definition for Frame buffer and Color
#define FB(addr, offset)		(*(volatile unsigned int *)((unsigned int *)(addr)+(unsigned int)(offset)))
#define RGB1(r, g, b)  (((((r) & 1) * 0x3ff << 20) | ((g) & 1) * 0x3ff << 10) | (((b) & 1) * 0x3ff))
#define RGB8(r, g, b)  ((((r) & 0xff) << 22) | (((g) & 0xff) << 12) | (((b) & 0xff) << 2))
#define RGB10(r, g, b) ((((r) & 0x3ff) << 20) | (((g) & 0x3ff) << 10) | ((b) & 0x3ff))

#define swap(a, b)		{ u32 c = a; a = b; b = c; }

// Definition for XGfxaccel IP
#define XGFXACCEL_MODE_FILLRECT		1
#define XGFXACCEL_MODE_LINE			2
#define XGFXACCEL_MODE_BITBLT		3

/*
 * Device instance definitions
 */
/* DMA channel setup
 */
XAxiVdma AxiVdma;
static XAxiVdma_DmaSetup ReadCfg;

/* Data address
 *
 * Read and write sub-frame use the same settings
 */
static u32 ReadFrameAddr;
static u32 WriteFrameAddr;
static u32 ResourceAddr;

static u32 BlockStartOffset;
static u32 BlockHoriz;
static u32 BlockVert;

/*
 * Accelerator instance
 */
XGfxaccel InstanceXGfxaccel;

/******************* Function Prototypes ************************************/

static int ReadSetup(XAxiVdma *InstancePtr);
static int StartTransfer(XAxiVdma *InstancePtr);
static int StopTransfer(XAxiVdma *InstancePtr);
static void SetupFrame(u32 baseAddr);

// LQ070 LCD display module control IP
#define RegLq070Out(offset)   (*(volatile unsigned int *)((XPAR_AXI_LQ070_OUT_0_S_AXI_BASEADDR)+(offset)))
#define Lq070Out_Ctrl		  (RegLq070Out(0x00))
#define   LQ070_PG_NONE		  0x00		// Pass-through
#define   LQ070_PG_CBAR		  0x01		// Color Bar pattern
#define   LQ070_PG_LAMP		  0x02		// Lamp pattern (64 step)
#define   LQ070_PG_HTCH		  0x03		// Hatch pattern

/*
-- ------------------------Address Info-------------------
-- 0x00 : Control signals
--        bit 0  - ap_start (Read/Write/COH)
--        bit 1  - ap_done (Read/COR)
--        bit 2  - ap_idle (Read)
--        bit 3  - ap_ready (Read)
--        bit 7  - auto_restart (Read/Write)
--        others - reserved
-- 0x04 : Global Interrupt Enable Register
-- 0x08 : IP Interrupt Enable Register (Read/Write)
-- 0x0c : IP Interrupt Status Register (Read/TOW)
-- 0x10 : Data signal of src_fb
--        bit 31~0 - src_fb[31:0] (Read/Write)
-- 0x18 : Data signal of x1
--        bit 15~0 - x1[15:0] (Read/Write)
-- 0x20 : Data signal of y1
--        bit 15~0 - y1[15:0] (Read/Write)
-- 0x28 : Data signal of dx
--        bit 15~0 - dx[15:0] (Read/Write)
-- 0x30 : Data signal of dy
--        bit 15~0 - dy[15:0] (Read/Write)
-- 0x38 : Data signal of dst_fb
--        bit 31~0 - dst_fb[31:0] (Read/Write)
-- 0x40 : Data signal of x2
--        bit 15~0 - x2[15:0] (Read/Write)
-- 0x48 : Data signal of y2
--        bit 15~0 - y2[15:0] (Read/Write)
-- 0x50 : Data signal of col
--        bit 31~0 - col[31:0] (Read/Write)
-- 0x58 : Data signal of mode
--        bit 7~0 - mode[7:0] (Read/Write)
*/

extern XGfxaccel_Config* XGfxaccel_LookupConfig(u16 DeviceId);
extern void XGfxaccel_Start(XGfxaccel *InstancePtr);
extern void XGfxaccel_Stop(XGfxaccel *InstancePtr);
extern u32 XGfxaccel_IsIdle(XGfxaccel *InstancePtr);
extern u32 XGfxaccel_IsReady(XGfxaccel *InstancePtr);
extern void XGfxaccel_Set_src_fb(XGfxaccel *InstancePtr, u32 Data);
extern void XGfxaccel_Set_x1(XGfxaccel *InstancePtr, u32 Data);
extern void XGfxaccel_Set_y1(XGfxaccel *InstancePtr, u32 Data);
extern void XGfxaccel_Set_dx(XGfxaccel *InstancePtr, u32 Data);
extern void XGfxaccel_Set_dy(XGfxaccel *InstancePtr, u32 Data);
extern void XGfxaccel_Set_dst_fb(XGfxaccel *InstancePtr, u32 Data);
extern void XGfxaccel_Set_x2(XGfxaccel *InstancePtr, u32 Data);
extern void XGfxaccel_Set_y2(XGfxaccel *InstancePtr, u32 Data);
extern void XGfxaccel_Set_col(XGfxaccel *InstancePtr, u32 Data);
extern void XGfxaccel_Set_mode(XGfxaccel *InstancePtr, u32 Data);

XGfxaccel_Config XGfxaccel_ConfigTable[XPAR_XGFXACCEL_NUM_INSTANCES] =
{
	{
		XPAR_XGFXACCEL_0_DEVICE_ID,
		XPAR_XGFXACCEL_0_S_AXI_CONTROL_BASEADDR
	}
};

XGfxaccel_Config *XGfxaccel_LookupConfig(u16 DeviceId) {
	XGfxaccel_Config *ConfigPtr = NULL;

	int Index;

	for (Index = 0; Index < XPAR_XGFXACCEL_NUM_INSTANCES; Index++) {
		if (XGfxaccel_ConfigTable[Index].DeviceId == DeviceId) {
			ConfigPtr = &XGfxaccel_ConfigTable[Index];
			break;
		}
	}

	return ConfigPtr;
}

int XGfxaccel_CfgInitialize(XGfxaccel *InstancePtr, XGfxaccel_Config *ConfigPtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(ConfigPtr != NULL);

    InstancePtr->Control_BaseAddress = ConfigPtr->Control_BaseAddress;
    InstancePtr->IsReady = XIL_COMPONENT_IS_READY;

    return XST_SUCCESS;
}

int XGfxaccel_Initialize(XGfxaccel *InstancePtr, u16 DeviceId) {
	XGfxaccel_Config *ConfigPtr;

	Xil_AssertNonvoid(InstancePtr != NULL);

	ConfigPtr = XGfxaccel_LookupConfig(DeviceId);
	if (ConfigPtr == NULL) {
		InstancePtr->IsReady = 0;
		return (XST_DEVICE_NOT_FOUND);
	}

	return XGfxaccel_CfgInitialize(InstancePtr, ConfigPtr);
}

void XGfxaccel_Start(XGfxaccel *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XGfxaccel_ReadReg(InstancePtr->Control_BaseAddress, XGFXACCEL_CONTROL_ADDR_AP_CTRL) & 0x80;
    XGfxaccel_WriteReg(InstancePtr->Control_BaseAddress, XGFXACCEL_CONTROL_ADDR_AP_CTRL, Data | 0x01);
}

void XGfxaccel_Stop(XGfxaccel *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XGfxaccel_ReadReg(InstancePtr->Control_BaseAddress, XGFXACCEL_CONTROL_ADDR_AP_CTRL) & 0x80;
    XGfxaccel_WriteReg(InstancePtr->Control_BaseAddress, XGFXACCEL_CONTROL_ADDR_AP_CTRL, Data);
}

u32 XGfxaccel_IsDone(XGfxaccel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XGfxaccel_ReadReg(InstancePtr->Control_BaseAddress, XGFXACCEL_CONTROL_ADDR_AP_CTRL);
    return (Data >> 1) & 0x1;
}

u32 XGfxaccel_IsIdle(XGfxaccel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XGfxaccel_ReadReg(InstancePtr->Control_BaseAddress, XGFXACCEL_CONTROL_ADDR_AP_CTRL);
    return (Data >> 2) & 0x1;
}

u32 XGfxaccel_IsReady(XGfxaccel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XGfxaccel_ReadReg(InstancePtr->Control_BaseAddress, XGFXACCEL_CONTROL_ADDR_AP_CTRL);
    // check ap_start to see if the pcore is ready for next input
    return !(Data & 0x1);
}

void XGfxaccel_Set_src_fb(XGfxaccel *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XGfxaccel_WriteReg(InstancePtr->Control_BaseAddress, XGFXACCEL_CONTROL_ADDR_SRC_FB_DATA, Data);
}

void XGfxaccel_Set_x1(XGfxaccel *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XGfxaccel_WriteReg(InstancePtr->Control_BaseAddress, XGFXACCEL_CONTROL_ADDR_X1_DATA, Data);
}

void XGfxaccel_Set_y1(XGfxaccel *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XGfxaccel_WriteReg(InstancePtr->Control_BaseAddress, XGFXACCEL_CONTROL_ADDR_Y1_DATA, Data);
}

void XGfxaccel_Set_dx(XGfxaccel *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XGfxaccel_WriteReg(InstancePtr->Control_BaseAddress, XGFXACCEL_CONTROL_ADDR_DX_DATA, Data);
}

void XGfxaccel_Set_dy(XGfxaccel *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XGfxaccel_WriteReg(InstancePtr->Control_BaseAddress, XGFXACCEL_CONTROL_ADDR_DY_DATA, Data);
}

void XGfxaccel_Set_dst_fb(XGfxaccel *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XGfxaccel_WriteReg(InstancePtr->Control_BaseAddress, XGFXACCEL_CONTROL_ADDR_DST_FB_DATA, Data);
}

void XGfxaccel_Set_x2(XGfxaccel *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XGfxaccel_WriteReg(InstancePtr->Control_BaseAddress, XGFXACCEL_CONTROL_ADDR_X2_DATA, Data);
}

void XGfxaccel_Set_y2(XGfxaccel *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XGfxaccel_WriteReg(InstancePtr->Control_BaseAddress, XGFXACCEL_CONTROL_ADDR_Y2_DATA, Data);
}

void XGfxaccel_Set_col(XGfxaccel *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XGfxaccel_WriteReg(InstancePtr->Control_BaseAddress, XGFXACCEL_CONTROL_ADDR_COL_DATA, Data);
}

void XGfxaccel_Set_mode(XGfxaccel *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XGfxaccel_WriteReg(InstancePtr->Control_BaseAddress, XGFXACCEL_CONTROL_ADDR_MODE_DATA, Data);
}

static void HwIpGfxaccel(u32 src_fb, uint16_t x1, uint16_t y1, uint16_t dx, uint16_t dy, u32 dst_fb, uint16_t x2, uint16_t y2, uint32_t col, uint8_t mode)
{
	XGfxaccel_Set_src_fb(&InstanceXGfxaccel, src_fb);
	XGfxaccel_Set_dst_fb(&InstanceXGfxaccel, dst_fb);
    // Invoke XTop accelerator
	XGfxaccel_Set_mode(&InstanceXGfxaccel, mode); // mode
	XGfxaccel_Set_col(&InstanceXGfxaccel, col); // col
	XGfxaccel_Set_x1(&InstanceXGfxaccel, x1); // x1
	XGfxaccel_Set_y1(&InstanceXGfxaccel, y1); // y1
	XGfxaccel_Set_dx(&InstanceXGfxaccel, dx); // dx
	XGfxaccel_Set_dy(&InstanceXGfxaccel, dy); // dy
	XGfxaccel_Set_x2(&InstanceXGfxaccel, x2); // x2
	XGfxaccel_Set_y2(&InstanceXGfxaccel, y2); // y2

//    xil_printf("Wait for Idle signal...");
    while (!XGfxaccel_IsIdle(&InstanceXGfxaccel));
//    xil_printf("Done.\r\n\r\n");
//    xil_printf("Send start XTop IP signal\r\n");
    XGfxaccel_Start(&InstanceXGfxaccel);
//    xil_printf("Wait for Ready signal...");
    while (!XGfxaccel_IsReady(&InstanceXGfxaccel));
    XGfxaccel_Stop(&InstanceXGfxaccel);
//    xil_printf("Done.\r\n\r\n");
}

static void XGfxaccel_FillRect(u32 fb, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t col)
{
	HwIpGfxaccel(0, x1, y1, 0, 0, fb, x2, y2, col, XGFXACCEL_MODE_FILLRECT);
}

static void XGfxaccel_DrawLine(u32 fb, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t col)
{
	HwIpGfxaccel(0, x1, y1, 0, 0, fb, x2, y2, col, XGFXACCEL_MODE_LINE);
}

static void XGfxaccel_BitBlt(u32 src_fb, uint16_t x1, uint16_t y1, uint16_t dx, uint16_t dy, u32 dst_fb, uint16_t x2, uint16_t y2)
{
	HwIpGfxaccel(src_fb, x1, y1, dx, dy, dst_fb, x2, y2, 0, XGFXACCEL_MODE_BITBLT);
}

typedef struct _pos {
	int x;
	int y;
} pos;

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
	XGfxaccel_DrawLine(WriteFrameAddr, points[0].x, points[0].y, points[1].x, points[1].y, 0xffffffff);
	for (i = 0; i < 24; i++)
	{
		XGfxaccel_DrawLine(WriteFrameAddr, points[i+1].x, points[i+1].y, points[i+2].x, points[i+2].y, 0xffffffff);
		XGfxaccel_DrawLine(WriteFrameAddr, points[i+2].x, points[i+2].y, points[i].x, points[i].y, 0x3ff00000);
	}
}

static void SetupFrame(u32 baseAddr)
{
	int i, j;
	unsigned int col, data;

	for (i = 0; i < 480 / 16; i++) {
		for (j = 0; j < 800 / 16; j++) {
			col = ((i) << 2) | (j);
			data = RGB1(col >> 2, col >> 1, col);
			XGfxaccel_FillRect(baseAddr, j * 16, i * 16, j * 16 + 15, i * 16 + 15, data);
		}
	}
}

int main()
{
	int i, j;
	u32 size = FRAME_HORIZONTAL_LEN * FRAME_VERTICAL_LEN;
	XAxiVdma_Config *Config;
	XAxiVdma_FrameCounter FrameCfg;
	int Status;

	ReadFrameAddr = READ_ADDRESS_BASE;
	WriteFrameAddr = WRITE_ADDRESS_BASE;
	BlockStartOffset = SUBFRAME_START_OFFSET;
	BlockHoriz = SUBFRAME_HORIZONTAL_SIZE;
	BlockVert = SUBFRAME_VERTICAL_SIZE;
	ResourceAddr = READ_ADDRESS_BASE + size;

	xil_printf("\r\n--- Entering main() --- \r\n");

	/* The information of the XAxiVdma_Config comes from hardware build.
	 * The user IP should pass this information to the AXI DMA core.
	 */
	Config = XAxiVdma_LookupConfig(DMA_DEVICE_ID);
	if (!Config) {
		xil_printf(
		    "No video DMA found for ID %d\r\n", DMA_DEVICE_ID);

		return XST_FAILURE;
	}

	/* Initialize DMA engine */
	Status = XAxiVdma_CfgInitialize(&AxiVdma, Config, Config->BaseAddress);
	if (Status != XST_SUCCESS) {

		xil_printf(
		    "Configuration Initialization failed %d\r\n", Status);

		return XST_FAILURE;
	}

	/* Setup frame counter and delay counter for both channels
	 *
	 * This is to monitor the progress of the test only
	 *
	 * WARNING: In free-run mode, interrupts may overwhelm the system.
	 * In that case, it is better to disable interrupts.
	 */
	FrameCfg.ReadFrameCount = NUMBER_OF_READ_FRAMES;
	FrameCfg.WriteFrameCount = NUMBER_OF_WRITE_FRAMES;
	FrameCfg.ReadDelayTimerCount = DELAY_TIMER_COUNTER;
	FrameCfg.WriteDelayTimerCount = DELAY_TIMER_COUNTER;

	Status = XAxiVdma_SetFrameCounter(&AxiVdma, &FrameCfg);
	if (Status != XST_SUCCESS) {
		xil_printf(
		    	"Set frame counter failed %d\r\n", Status);

		if(Status == XST_VDMA_MISMATCH_ERROR)
			xil_printf("DMA Mismatch Error\r\n");

		return XST_FAILURE;
	}

	/*
	 * Setup your video IP that reads from the memory
	 */

	/* Setup the read channel
	 */
	Status = ReadSetup(&AxiVdma);
	if (Status != XST_SUCCESS) {
		xil_printf(
		    	"Read channel setup failed %d\r\n", Status);
		if(Status == XST_VDMA_MISMATCH_ERROR)
			xil_printf("DMA Mismatch Error\r\n");

		return XST_FAILURE;
	}

	/* Start the DMA engine to transfer
	 */
	Status = StartTransfer(&AxiVdma);
	if (Status != XST_SUCCESS) {
		if(Status == XST_VDMA_MISMATCH_ERROR)
			xil_printf("DMA Mismatch Error\r\n");
		return XST_FAILURE;
	}
	xil_printf("Test passed\r\n");

    Lq070Out_Ctrl = 0;
    print("Hello World\n\r");

    xil_printf("Initialize XGfxaccel instance\r\n");
    XGfxaccel_Initialize(&InstanceXGfxaccel, XPAR_XGFXACCEL_0_DEVICE_ID);

    // Clear frame buffer
    xil_printf("Clear frame buffer\r\n");
    for (i = 0; i < 480; i++)
    {
    	for (j = 0; j < 800; j++)
		{
			FB(WriteFrameAddr, (i*800+j)) = 1;
		}
    }
    Xil_DCacheFlushRange(WriteFrameAddr, size);

    // Setup Resource frame
    xil_printf("Setup Resource frame buffer\r\n");
    SetupFrame(ResourceAddr);
    Xil_DCacheFlushRange(ResourceAddr, size);
//    SetupFrame(WriteFrameAddr);
//    Xil_DCacheFlushRange(WriteFrameAddr, size);

    xil_printf("BitBlt Resource frame buffer to main frame buffer\r\n");
    for (i = 0; i < 480 / 16; i++)
    {
    	for (j = 0; j < 800 / 16; j++)
    	{
    		XGfxaccel_BitBlt(ResourceAddr, j * 16, (i /*% 15*/) * 16, 16, 16, WriteFrameAddr, j * 16, i * 16);
    	}
    }

    // Invoke fill rectangle accelerator
    XGfxaccel_FillRect(WriteFrameAddr, 80, 80, 719, 399, RGB8(255, 255, 255));
    // Invoke fill rectangle accelerator
    XGfxaccel_FillRect(WriteFrameAddr,  83,  83, 716, 396, RGB8(0, 64, 255));
    // Invoke fill rectangle accelerator
    XGfxaccel_FillRect(WriteFrameAddr,  80, 240, 719, 241, RGB8(255, 255, 255));
    // Invoke fill rectangle accelerator
    XGfxaccel_FillRect(WriteFrameAddr, 480, 240, 481, 399, RGB8(255, 255, 255));

    // Invalidate frame buffer to reload updated data
    Xil_DCacheInvalidateRange(ReadFrameAddr, size);

    // Output results
    xil_printf("Read Frame address: 0x%x", ReadFrameAddr);
    for (i = 0; i < 8; i++)
    {
    	xil_printf("\r\nfb(%03d): ", i);
    	for (j = 0; j < 16; j++)
		{
    		xil_printf("%02x ", FB(ReadFrameAddr, ((i+76)*800+(j+76))) & 0xFF);
		}
    }
	xil_printf("\r\n");

	drawTrianglePolygons();

    xil_printf("--- Exiting main() --- \r\n");

	return 0;
}

/*****************************************************************************/
/**
*
* This function sets up the read channel
*
* @param	InstancePtr is the instance pointer to the DMA engine.
*
* @return	XST_SUCCESS if the setup is successful, XST_FAILURE otherwise.
*
* @note		None.
*
******************************************************************************/
static int ReadSetup(XAxiVdma *InstancePtr)
{
	int Index;
	u32 Addr;
	int Status;

	ReadCfg.VertSizeInput = FRAME_VERTICAL_LEN;
	ReadCfg.HoriSizeInput = FRAME_HORIZONTAL_LEN;

	ReadCfg.Stride = FRAME_HORIZONTAL_LEN;
	ReadCfg.FrameDelay = 0;  /* This example does not test frame delay */

	ReadCfg.EnableCircularBuf = 1;
	ReadCfg.EnableSync = 1;  /* With Gen-Lock */

	ReadCfg.PointNum = 0;
	ReadCfg.EnableFrameCounter = 0; /* Endless transfers */

	ReadCfg.FixedFrameStoreAddr = 0; /* We are not doing parking */

	Status = XAxiVdma_DmaConfig(InstancePtr, XAXIVDMA_READ, &ReadCfg);
	if (Status != XST_SUCCESS) {
		xil_printf(
		    "Read channel config failed %d\r\n", Status);

		return XST_FAILURE;
	}

	/* Initialize buffer addresses
	 *
	 * These addresses are physical addresses
	 */
	Addr = READ_ADDRESS_BASE + BlockStartOffset;
	for(Index = 0; Index < NUMBER_OF_READ_FRAMES; Index++) {
		ReadCfg.FrameStoreStartAddr[Index] = Addr;

		Addr += FRAME_HORIZONTAL_LEN * FRAME_VERTICAL_LEN;
	}

	Addr = READ_ADDRESS_BASE + BlockStartOffset;
	//SetupFrame(Addr, 0);

	/* Set the buffer addresses for transfer in the DMA engine
	 * The buffer addresses are physical addresses
	 */
	Status = XAxiVdma_DmaSetBufferAddr(InstancePtr, XAXIVDMA_READ,
			ReadCfg.FrameStoreStartAddr);
	if (Status != XST_SUCCESS) {
		xil_printf(
		    "Read channel set buffer address failed %d\r\n", Status);

		return XST_FAILURE;
	}

	XAxiVdma_SetFrmStore(InstancePtr, 3, XAXIVDMA_READ);

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function starts the DMA transfers. Since the DMA engine is operating
* in circular buffer mode, video frames will be transferred continuously.
*
* @param	InstancePtr points to the DMA engine instance
*
* @return	XST_SUCCESS if both read and write start succesfully
*		XST_FAILURE if one or both directions cannot be started
*
* @note		None.
*
******************************************************************************/
static int StartTransfer(XAxiVdma *InstancePtr)
{
	int Status;

	Status = XAxiVdma_DmaStart(InstancePtr, XAXIVDMA_READ);
	if (Status != XST_SUCCESS) {
		xil_printf(
		    "Start read transfer failed %d\r\n", Status);

		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function stops the DMA transfers.
*
* @param	InstancePtr points to the DMA engine instance
*
* @return	No returned value
*
* @note		None.
*
******************************************************************************/
static int StopTransfer(XAxiVdma *InstancePtr)
{
	XAxiVdma_DmaStop(InstancePtr, XAXIVDMA_READ);

	return XST_SUCCESS;
}
