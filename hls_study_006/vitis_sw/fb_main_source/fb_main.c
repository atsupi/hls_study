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
#include "xtop.h"

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

XTop InstanceXTop;

int XTop_CfgInitialize(XTop *InstancePtr, XTop_Config *ConfigPtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(ConfigPtr != NULL);

    InstancePtr->Control_BaseAddress = ConfigPtr->Control_BaseAddress;
    InstancePtr->IsReady = XIL_COMPONENT_IS_READY;

    return XST_SUCCESS;
}

void XTop_Start(XTop *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XTop_ReadReg(InstancePtr->Control_BaseAddress, XTOP_CONTROL_ADDR_AP_CTRL) & 0x80;
    XTop_WriteReg(InstancePtr->Control_BaseAddress, XTOP_CONTROL_ADDR_AP_CTRL, Data | 0x01);
}

void XTop_Stop(XTop *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XTop_ReadReg(InstancePtr->Control_BaseAddress, XTOP_CONTROL_ADDR_AP_CTRL) & 0x80;
    XTop_WriteReg(InstancePtr->Control_BaseAddress, XTOP_CONTROL_ADDR_AP_CTRL, Data);
}

u32 XTop_IsDone(XTop *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XTop_ReadReg(InstancePtr->Control_BaseAddress, XTOP_CONTROL_ADDR_AP_CTRL);
    return (Data >> 1) & 0x1;
}

u32 XTop_IsIdle(XTop *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XTop_ReadReg(InstancePtr->Control_BaseAddress, XTOP_CONTROL_ADDR_AP_CTRL);
    return (Data >> 2) & 0x1;
}

u32 XTop_IsReady(XTop *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XTop_ReadReg(InstancePtr->Control_BaseAddress, XTOP_CONTROL_ADDR_AP_CTRL);
    // check ap_start to see if the pcore is ready for next input
    return !(Data & 0x1);
}

void XTop_Set_fb(XTop *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XTop_WriteReg(InstancePtr->Control_BaseAddress, XTOP_CONTROL_ADDR_FB_DATA, Data);
}

u32 XTop_Get_fb(XTop *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XTop_ReadReg(InstancePtr->Control_BaseAddress, XTOP_CONTROL_ADDR_FB_DATA);
    return Data;
}

XTop_Config XTop_ConfigTable[XPAR_XTOP_NUM_INSTANCES] =
{
	{
		XPAR_XTOP_0_DEVICE_ID,
		XPAR_XTOP_0_S_AXI_CONTROL_BASEADDR
	}
};

XTop_Config *XTop_LookupConfig(u16 DeviceId) {
	XTop_Config *ConfigPtr = NULL;

	int Index;

	for (Index = 0; Index < XPAR_XTOP_NUM_INSTANCES; Index++) {
		if (XTop_ConfigTable[Index].DeviceId == DeviceId) {
			ConfigPtr = &XTop_ConfigTable[Index];
			break;
		}
	}

	return ConfigPtr;
}

int XTop_Initialize(XTop *InstancePtr, u16 DeviceId) {
	XTop_Config *ConfigPtr;

	Xil_AssertNonvoid(InstancePtr != NULL);

	ConfigPtr = XTop_LookupConfig(DeviceId);
	if (ConfigPtr == NULL) {
		InstancePtr->IsReady = 0;
		return (XST_DEVICE_NOT_FOUND);
	}

	return XTop_CfgInitialize(InstancePtr, ConfigPtr);
}

int main()
{
	int i, j;
	u32 size = FRAME_HORIZONTAL_LEN * FRAME_VERTICAL_LEN;

	ReadFrameAddr = READ_ADDRESS_BASE;
	WriteFrameAddr = WRITE_ADDRESS_BASE;
	BlockStartOffset = SUBFRAME_START_OFFSET;
	BlockHoriz = SUBFRAME_HORIZONTAL_SIZE;
	BlockVert = SUBFRAME_VERTICAL_SIZE;

	xil_printf("\r\n--- Entering main() --- \r\n");
    print("Hello World\n\r");

    xil_printf("Initialize XTop driver\r\n");
    XTop_Initialize(&InstanceXTop, XPAR_XTOP_0_DEVICE_ID);
    xil_printf("Set frame buffer base adddress offset\r\n");
    XTop_Set_fb(&InstanceXTop, WriteFrameAddr);

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
    xil_printf("Wait for Idle signal...");
    while (!XTop_IsIdle(&InstanceXTop));
    xil_printf("Done.\r\n\r\n");
    xil_printf("Send start XTop IP signal\r\n");
    XTop_Start(&InstanceXTop);
    xil_printf("Wait for Ready signal...");
    while (!XTop_IsReady(&InstanceXTop));
    XTop_Stop(&InstanceXTop);
    xil_printf("Done.\r\n\r\n");
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
	xil_printf("--- Exiting main() --- \r\n");

	return 0;
}
