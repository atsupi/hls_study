/******************************************************
 *    Filename:     vdma.c 
 *     Purpose:     vdma controller driver 
 *  Created on: 	2015/04/05 
 * Modified on: 	2021/01/18 
 *      Author: 	atsupi.com 
 *     Version:		1.10 
 ******************************************************/

//#define DEBUG

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "azplf_bsp.h"
#include "vdma.h"

#define MM2S_VDMACR_INITIAL		0x00000189	// GenSrc=Internal;Genlock;Circular;Run
#define S2MM_VDMACR_INITIAL		0x00000289	// GenSrc=Internal;Genlock;Circular;Run

static u32 BlockHorizWrite;
static u32 BlockVertWrite;
static u32 BlockHorizRead;
static u32 BlockVertRead;
static u32 page_size;
static u32 VdmaAddress = 0;
u32 frame_page = 0x00200000; // 2MB page

static u32 VirtReadFrameAddr[NUMBER_OF_READ_FRAMES];
static u32 PhysReadFrameAddr[NUMBER_OF_READ_FRAMES];

uint32_t vdma_read_reg(u32 adr, u32 offset)
{
	if (!adr)
	{
		printf("vdma_read_reg baseaddress error!\n");
		return 0;
	}
	return (*(volatile unsigned int *)(adr + offset));
}

void vdma_write_reg(u32 adr, u32 offset, u32 value)
{
	if (!adr)
	{
		printf("vdma_write_reg baseaddress error!\n");
		return;
	}
	*(volatile unsigned int *)(adr + offset) = value;
#ifdef DEBUG
	u32 data = vdma_read_reg(adr, offset);
	printf("vdma_read_reg(offset:0x%04x) %x : readreg %x\n", offset, value, data);
#endif
}

void vdma_init(void)
{
	int i;
	void *ptr;
	int memfd;

	page_size = sysconf(_SC_PAGESIZE);
	printf("File page size=0x%08x (%dKB)\n", page_size, page_size>>10);

	BlockHorizWrite = SUBFRAME_HORIZONTAL_SIZE;
	BlockVertWrite = SUBFRAME_VERTICAL_SIZE;
	BlockHorizRead = FRAME_HORIZONTAL_LEN;
	BlockVertRead = FRAME_VERTICAL_LEN;

	for (i = 0; i < NUMBER_OF_READ_FRAMES; i++)
	{
		VirtReadFrameAddr[i] = 0;
		PhysReadFrameAddr[i] = 0;
	}

	memfd = open("/dev/mem", O_RDWR);
	if (memfd < 1)
		VdmaAddress = 0;

	ptr = mmap(NULL, page_size, PROT_READ|PROT_WRITE, MAP_SHARED, memfd, VDMA_BASEADDRESS);
	VdmaAddress = (u32)ptr;

	printf("Mapping I/O: 0x%08x to vmem: 0x%08x\n", VDMA_BASEADDRESS, (u32)ptr);
	if (VdmaAddress)
	{
		u32 data = vdma_read_reg(VdmaAddress, VDMA_VERSION);
		printf("XVdma_Version = %08x\n", data);
	}
	close(memfd);

	// Set Control Register
	vdma_write_reg(VdmaAddress, MM2S_VDMACR, 0x00000002);	// IRQFrameCount;Circular;Run
	vdma_write_reg(VdmaAddress, S2MM_VDMACR, 0x00000002);	// IRQFrameCount;Circular;Run
	// Set VSize to start VDMA channel
	vdma_write_reg(VdmaAddress, MM2S_VSIZE, 0);
	// Set VSize to start VDMA channel
	vdma_write_reg(VdmaAddress, S2MM_VSIZE, 0);
}

void vdma_deinit(void)
{
	int i;
	for (i = 0; i < NUMBER_OF_READ_FRAMES; i++)
		if (VirtReadFrameAddr[i])
			munmap((void *)VirtReadFrameAddr[i], frame_page);

	if (VdmaAddress)
		munmap((void *)VdmaAddress, page_size);
}

int vdma_config(int mode)
{
	u32 data;
	printf("VDMA Configuration(%d)\n", mode);
	if (mode == VDMA_READ)
	{
		// Set Control Register
		vdma_write_reg(VdmaAddress, MM2S_VDMACR, MM2S_VDMACR_INITIAL);	// IRQFrameCount;Circular;Run
	}
	else if (mode == VDMA_WRITE)
	{
		// Set Control Register
		vdma_write_reg(VdmaAddress, S2MM_VDMACR, S2MM_VDMACR_INITIAL);	// IRQFrameCount;Circular;Run
	}
	data = vdma_read_reg(VdmaAddress, MM2S_VDMACR);
	printf("vdma_read_reg(%04x): %08x\n", MM2S_VDMACR, data);
	data = vdma_read_reg(VdmaAddress, MM2S_VDMASR);
	printf("vdma_read_reg(%04x): %08x\n", MM2S_VDMASR, data);
	data = vdma_read_reg(VdmaAddress, S2MM_VDMACR);
	printf("vdma_read_reg(%04x): %08x\n", S2MM_VDMACR, data);
	data = vdma_read_reg(VdmaAddress, S2MM_VDMASR);
	printf("vdma_read_reg(%04x): %08x\n", S2MM_VDMASR, data);
	return PST_SUCCESS;
}

int vdma_dma_setbuffer(int mode)
{
	int i;
	u32 data;
	printf("VDMA Set Buffer Address(%d)\n", mode);
	if (mode == VDMA_READ)
	{
		for (i = 0; i < NUMBER_OF_READ_FRAMES; i++)
		{
			// Set Physical Frame Buffer Address
			vdma_write_reg(VdmaAddress, MM2S_START_ADDRESS + i*4, PhysReadFrameAddr[i]);
		}
		data = vdma_read_reg(VdmaAddress, MM2S_START_ADDRESS);
		printf("vdma_read_reg(%04x): %08x\n", MM2S_START_ADDRESS, data);
	}
	else if (mode == VDMA_WRITE)
	{
		// ToDo:
		for (i = 0; i < NUMBER_OF_WRITE_FRAMES; i++)
		{
			// Set Physical Frame Buffer Address
			vdma_write_reg(VdmaAddress, S2MM_START_ADDRESS + i*4, PhysReadFrameAddr[i]);
		}
		data = vdma_read_reg(VdmaAddress, S2MM_START_ADDRESS);
		printf("vdma_read_reg(%04x): %08x\n", S2MM_START_ADDRESS, data);
	}
	return PST_SUCCESS;
}

void vdma_set_frame_address(int index, u32 addr)
{
	int memfd;

	void *ptr;
	if (PhysReadFrameAddr[index])
	{
		printf("Mapping frame address: 0x%08x is already set\n", addr);
		return;
	}

	memfd = open("/dev/mem", O_RDWR);
	if (memfd < 1)
		VirtReadFrameAddr[index] = 0;

	ptr = mmap(NULL, frame_page, PROT_READ|PROT_WRITE, MAP_SHARED, memfd, addr);

	printf("Mapping I/O: 0x%08x to vmem: 0x%08x\n", addr, (u32)ptr);

	close(memfd);

	PhysReadFrameAddr[index] = addr;
	VirtReadFrameAddr[index] = (u32)ptr;
}

int vdma_start(int mode)
{
	u32 data;
	int frmdly = 0;
	printf("VDMA Start(%d)\n", mode);
	if (mode == VDMA_READ)
	{
		// Set DelayStride/HSize
		vdma_write_reg(VdmaAddress, MM2S_FRMDLY_STRIDE, ((frmdly & 0x1F) << 24) | FRAME_HORIZONTAL_LEN);
		vdma_write_reg(VdmaAddress, MM2S_HSIZE, BlockHorizRead);
		// Set VSize to start VDMA channel
		vdma_write_reg(VdmaAddress, MM2S_VSIZE, BlockVertRead);

		data = vdma_read_reg(VdmaAddress, MM2S_VSIZE);
		printf("vdma_read_reg(%04x): %08x\n", MM2S_VSIZE, data);
		data = vdma_read_reg(VdmaAddress, MM2S_HSIZE);
		printf("vdma_read_reg(%04x): %08x\n", MM2S_HSIZE, data);
		data = vdma_read_reg(VdmaAddress, MM2S_FRMDLY_STRIDE);
		printf("vdma_read_reg(%04x): %08x\n", MM2S_FRMDLY_STRIDE, data);
	}
	else if (mode == VDMA_WRITE)
	{
		// ToDo:
		// Set DelayStride/HSize
		vdma_write_reg(VdmaAddress, S2MM_FRMDLY_STRIDE, ((frmdly & 0x1F) << 24) | FRAME_HORIZONTAL_LEN);
		vdma_write_reg(VdmaAddress, S2MM_HSIZE, BlockHorizWrite);
		// Set VSize to start VDMA channel
		vdma_write_reg(VdmaAddress, S2MM_VSIZE, BlockVertWrite);

		data = vdma_read_reg(VdmaAddress, S2MM_VSIZE);
		printf("vdma_read_reg(%04x): %08x\n", S2MM_VSIZE, data);
		data = vdma_read_reg(VdmaAddress, S2MM_HSIZE);
		printf("vdma_read_reg(%04x): %08x\n", S2MM_HSIZE, data);
		data = vdma_read_reg(VdmaAddress, S2MM_FRMDLY_STRIDE);
		printf("vdma_read_reg(%04x): %08x\n", S2MM_FRMDLY_STRIDE, data);
	}
	return PST_SUCCESS;
}

int vdma_stop(int mode)
{
	u32 data;
	printf("VDMA Stop(%d)\n", mode);

	if (mode == VDMA_READ)
	{
		// Set Control Register
		data = vdma_read_reg(VdmaAddress, MM2S_VDMACR);
		data &= 0xFFFFFFFE; // reset RS bit
		vdma_write_reg(VdmaAddress, MM2S_VDMACR, data);	// IRQFrameCount;Circular;Run
	}
	else if (mode == VDMA_WRITE)
	{
		// Set Control Register
		data = vdma_read_reg(VdmaAddress, S2MM_VDMACR);
		data &= 0xFFFFFFFE; // reset RS bit
		vdma_write_reg(VdmaAddress, S2MM_VDMACR, data);	// IRQFrameCount;Circular;Run
	}
	data = vdma_read_reg(VdmaAddress, MM2S_VDMACR);
	printf("vdma_read_reg(%04x): %08x\n", MM2S_VDMACR, data);
	data = vdma_read_reg(VdmaAddress, MM2S_VDMASR);
	printf("vdma_read_reg(%04x): %08x\n", MM2S_VDMASR, data);
	data = vdma_read_reg(VdmaAddress, S2MM_VDMACR);
	printf("vdma_read_reg(%04x): %08x\n", S2MM_VDMACR, data);
	data = vdma_read_reg(VdmaAddress, S2MM_VDMASR);
	printf("vdma_read_reg(%04x): %08x\n", S2MM_VDMASR, data);
	return PST_SUCCESS;
}

u32 vdma_get_frame_address(int fbnum)
{
	return (VirtReadFrameAddr[fbnum]);
}

int vdma_start_parking(int mode, int fbnum)
{
	u32 data;
#ifdef DEBUG
	printf("VDMA Start Parking(%d): %d\n", mode, fbnum);
#endif

	if (mode == VDMA_READ)
	{
		// Set Control Register
		data = vdma_read_reg(VdmaAddress, VDMA_PARKPTR);
		data &= 0xFFFFFFE0; // reset PARKPTR bits
		data |= fbnum;
		vdma_write_reg(VdmaAddress, VDMA_PARKPTR, data);
		// Set Control Register
		data = vdma_read_reg(VdmaAddress, MM2S_VDMACR);
		data &= 0xFFFFFFFD; // reset TAIL_EN bit
		vdma_write_reg(VdmaAddress, MM2S_VDMACR, data);
	}
	else if (mode == VDMA_WRITE)
	{
		// Set Control Register
		data = vdma_read_reg(VdmaAddress, VDMA_PARKPTR);
		data &= 0xFFFFE0FF; // reset PARKPTR bit
		data |= (fbnum << 8);
		vdma_write_reg(VdmaAddress, VDMA_PARKPTR, data);
		// Set Control Register
		data = vdma_read_reg(VdmaAddress, S2MM_VDMACR);
		data &= 0xFFFFFFFD; // reset TAIL_EN bit
		vdma_write_reg(VdmaAddress, S2MM_VDMACR, data);
	}
	return PST_SUCCESS;
}

int vdma_stop_parking(int mode)
{
	u32 data;
#ifdef DEBUG
	printf("VDMA Stop Parking(%d)\n", mode);
#endif

	if (mode == VDMA_READ)
	{
		// Set Control Register
		data = vdma_read_reg(VdmaAddress, MM2S_VDMACR);
		data |= 0x02; // set TAIL_EN bit
		vdma_write_reg(VdmaAddress, MM2S_VDMACR, data);
	}
	else if (mode == VDMA_WRITE)
	{
		// Set Control Register
		data = vdma_read_reg(VdmaAddress, S2MM_VDMACR);
		data |= 0x02; // set TAIL_EN bit
		vdma_write_reg(VdmaAddress, S2MM_VDMACR, data);
	}
	return PST_SUCCESS;
}
