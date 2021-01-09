/******************************************************
 *  Filename: main.c 
 *  Created on: 	2020/09/28
 *      Author: 	atsupi
 *     Version:		0.1
 ******************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "xil_printf.h"
#include "xparameters.h"
#include "xaxivdma.h"
#include "xil_cache.h"
#include "xdrawline.h"
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
#define NUMBER_OF_READ_FRAMES	1
#define NUMBER_OF_WRITE_FRAMES	1

#define DELAY_TIMER_COUNTER	10

#define FB(addr, offset)		(*(volatile unsigned int *)((unsigned int *)(addr)+(unsigned int)(offset)))
#define RGB1(r, g, b)  (((((r) & 1) * 0x3ff << 20) | ((g) & 1) * 0x3ff << 10) | (((b) & 1) * 0x3ff))
#define RGB8(r, g, b)  ((((r) & 0xff) << 22) | (((g) & 0xff) << 12) | (((b) & 0xff) << 2))
#define RGB10(r, g, b) ((((r) & 0x3ff) << 20) | (((g) & 0x3ff) << 10) | ((b) & 0x3ff))

#define swap(a, b)		{ u32 c = a; a = b; b = c; }

/*
 * Device instance definitions
 */
XAxiVdma AxiVdma;
/* Data address
 *
 * Read and write sub-frame use the same settings
 */
static u32 ReadFrameAddr;
static u32 WriteFrameAddr;

static u32 BlockStartOffset;
static u32 BlockHoriz;
static u32 BlockVert;

XDrawline InstanceXDrawline;
/* DMA channel setup
 */
static XAxiVdma_DmaSetup ReadCfg;

/******************* Function Prototypes ************************************/

static int ReadSetup(XAxiVdma *InstancePtr);
static int StartTransfer(XAxiVdma *InstancePtr);
static int StopTransfer(XAxiVdma *InstancePtr);
//static void SetupFrame(unsigned int baseAddr, int mode);

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
-- 0x10 : Data signal of fb
--        bit 31~0 - fb[31:0] (Read/Write)
-- 0x18 : Data signal of col
--        bit 31~0 - col[31:0] (Read/Write)
-- 0x20 : Data signal of x1
--        bit 15~0 - x1[15:0] (Read/Write)
-- 0x28 : Data signal of y1
--        bit 15~0 - y1[15:0] (Read/Write)
-- 0x30 : Data signal of x2
--        bit 15~0 - x2[15:0] (Read/Write)
-- 0x38 : Data signal of y2
--        bit 15~0 - y2[15:0] (Read/Write)
*/

extern XDrawline_Config* XDrawline_LookupConfig(u16 DeviceId);
extern void XDrawline_Start(XDrawline *InstancePtr);
extern void XDrawline_Stop(XDrawline *InstancePtr);
extern u32 XDrawline_IsIdle(XDrawline *InstancePtr);
extern u32 XDrawline_IsReady(XDrawline *InstancePtr);
extern void XDrawline_Set_fb(XDrawline *InstancePtr, u32 Data);
extern void XDrawline_Set_x1(XDrawline *InstancePtr, u32 Data);
extern void XDrawline_Set_y1(XDrawline *InstancePtr, u32 Data);
extern void XDrawline_Set_x2(XDrawline *InstancePtr, u32 Data);
extern void XDrawline_Set_y2(XDrawline *InstancePtr, u32 Data);
extern void XDrawline_Set_col(XDrawline *InstancePtr, u32 Data);

XDrawline_Config XDrawline_ConfigTable[XPAR_XDRAWLINE_NUM_INSTANCES] =
{
	{
		XPAR_XDRAWLINE_0_DEVICE_ID,
		XPAR_XDRAWLINE_0_S_AXI_CONTROL_BASEADDR
	}
};

XDrawline_Config *XDrawline_LookupConfig(u16 DeviceId) {
	XDrawline_Config *ConfigPtr = NULL;

	int Index;

	for (Index = 0; Index < XPAR_XDRAWLINE_NUM_INSTANCES; Index++) {
		if (XDrawline_ConfigTable[Index].DeviceId == DeviceId) {
			ConfigPtr = &XDrawline_ConfigTable[Index];
			break;
		}
	}

	return ConfigPtr;
}

int XDrawline_CfgInitialize(XDrawline *InstancePtr, XDrawline_Config *ConfigPtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(ConfigPtr != NULL);

    InstancePtr->Control_BaseAddress = ConfigPtr->Control_BaseAddress;
    InstancePtr->IsReady = XIL_COMPONENT_IS_READY;

    return XST_SUCCESS;
}

int XDrawline_Initialize(XDrawline *InstancePtr, u16 DeviceId) {
	XDrawline_Config *ConfigPtr;

	Xil_AssertNonvoid(InstancePtr != NULL);

	ConfigPtr = XDrawline_LookupConfig(DeviceId);
	if (ConfigPtr == NULL) {
		InstancePtr->IsReady = 0;
		return (XST_DEVICE_NOT_FOUND);
	}

	return XDrawline_CfgInitialize(InstancePtr, ConfigPtr);
}

void XDrawline_Start(XDrawline *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDrawline_ReadReg(InstancePtr->Control_BaseAddress, XDRAWLINE_CONTROL_ADDR_AP_CTRL) & 0x80;
    XDrawline_WriteReg(InstancePtr->Control_BaseAddress, XDRAWLINE_CONTROL_ADDR_AP_CTRL, Data | 0x01);
}

void XDrawline_Stop(XDrawline *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDrawline_ReadReg(InstancePtr->Control_BaseAddress, XDRAWLINE_CONTROL_ADDR_AP_CTRL) & 0x80;
    XDrawline_WriteReg(InstancePtr->Control_BaseAddress, XDRAWLINE_CONTROL_ADDR_AP_CTRL, Data);
}

u32 XDrawline_IsDone(XDrawline *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDrawline_ReadReg(InstancePtr->Control_BaseAddress, XDRAWLINE_CONTROL_ADDR_AP_CTRL);
    return (Data >> 1) & 0x1;
}

u32 XDrawline_IsIdle(XDrawline *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDrawline_ReadReg(InstancePtr->Control_BaseAddress, XDRAWLINE_CONTROL_ADDR_AP_CTRL);
    return (Data >> 2) & 0x1;
}

u32 XDrawline_IsReady(XDrawline *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDrawline_ReadReg(InstancePtr->Control_BaseAddress, XDRAWLINE_CONTROL_ADDR_AP_CTRL);
    // check ap_start to see if the pcore is ready for next input
    return !(Data & 0x1);
}

void XDrawline_Set_fb(XDrawline *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDrawline_WriteReg(InstancePtr->Control_BaseAddress, XDRAWLINE_CONTROL_ADDR_FB_DATA, Data);
}

void XDrawline_Set_x1(XDrawline *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDrawline_WriteReg(InstancePtr->Control_BaseAddress, XDRAWLINE_CONTROL_ADDR_X1_DATA, Data);
}

void XDrawline_Set_y1(XDrawline *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDrawline_WriteReg(InstancePtr->Control_BaseAddress, XDRAWLINE_CONTROL_ADDR_Y1_DATA, Data);
}

void XDrawline_Set_x2(XDrawline *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDrawline_WriteReg(InstancePtr->Control_BaseAddress, XDRAWLINE_CONTROL_ADDR_X2_DATA, Data);
}

void XDrawline_Set_y2(XDrawline *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDrawline_WriteReg(InstancePtr->Control_BaseAddress, XDRAWLINE_CONTROL_ADDR_Y2_DATA, Data);
}

void XDrawline_Set_col(XDrawline *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDrawline_WriteReg(InstancePtr->Control_BaseAddress, XDRAWLINE_CONTROL_ADDR_COL_DATA, Data);
}

static void HwIpDrawLine(u32 fb, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t col)
{
	XDrawline_Set_fb(&InstanceXDrawline, fb);
    // Invoke XTop accelerator
	XDrawline_Set_col(&InstanceXDrawline, col); // col
	XDrawline_Set_x1(&InstanceXDrawline, x1); // x1
	XDrawline_Set_y1(&InstanceXDrawline, y1); // y1
	XDrawline_Set_x2(&InstanceXDrawline, x2); // x2
	XDrawline_Set_y2(&InstanceXDrawline, y2); // y2

//    xil_printf("Wait for Idle signal...");
    while (!XDrawline_IsIdle(&InstanceXDrawline));
//    xil_printf("Done.\r\n\r\n");
//    xil_printf("Send start XTop IP signal\r\n");
    XDrawline_Start(&InstanceXDrawline);
//    xil_printf("Wait for Ready signal...");
    while (!XDrawline_IsReady(&InstanceXDrawline));
    XDrawline_Stop(&InstanceXDrawline);
//    xil_printf("Done.\r\n\r\n");
}

int main()
{
	int i, j;
	u32 size = FRAME_HORIZONTAL_LEN * FRAME_VERTICAL_LEN;
	XAxiVdma_Config *Config;
	XAxiVdma_FrameCounter FrameCfg;
	int Status;
	u32 x1, y1, x2, y2, col;

	ReadFrameAddr = READ_ADDRESS_BASE;
	WriteFrameAddr = WRITE_ADDRESS_BASE;
	BlockStartOffset = SUBFRAME_START_OFFSET;
	BlockHoriz = SUBFRAME_HORIZONTAL_SIZE;
	BlockVert = SUBFRAME_VERTICAL_SIZE;

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

    xil_printf("Initialize XTop driver\r\n");
    XDrawline_Initialize(&InstanceXDrawline, XPAR_XDRAWLINE_0_DEVICE_ID);

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

    // Invoke XTop accelerator
    HwIpDrawLine(WriteFrameAddr, 80, 80, 719, 399, RGB8(255, 255, 255));
    // Invoke XTop accelerator
    HwIpDrawLine(WriteFrameAddr,  83,  83, 716, 396, RGB8(0, 64, 255));
    // Invoke XTop accelerator
    HwIpDrawLine(WriteFrameAddr,  80, 240, 719, 241, RGB8(255, 255, 255));
    // Invoke XTop accelerator
    HwIpDrawLine(WriteFrameAddr, 480, 240, 481, 399, RGB8(255, 255, 255));

    // Invalidate frame buffer to reload updated data
    Xil_DCacheInvalidateRange(ReadFrameAddr, size);

    // Output results
    xil_printf("Read Frame address: 0x%x", ReadFrameAddr);
    for (i = 0; i < 8; i++)
    {
    	xil_printf("\r\nfb(%03d): ", i);
    	for (j = 0; j < 32; j++)
		{
    		xil_printf("%02x ", FB(ReadFrameAddr, ((i+76)*800+(j+76))) & 0xFF);
		}
    }
	xil_printf("\r\n");

	xil_printf("RAND_MAX = 0x%0x\r\n", RAND_MAX);
	for (i = 0; i < 1024; i++)
	{
		x1 = rand() % 800;
		y1 = rand() % 480;
		x2 = rand() % 800;
		y2 = rand() % 480;
		col = RGB1(rand() & 1, rand() & 1, rand() & 1);
		HwIpDrawLine(WriteFrameAddr, x1, y1, x2, y2, col);
	}
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
