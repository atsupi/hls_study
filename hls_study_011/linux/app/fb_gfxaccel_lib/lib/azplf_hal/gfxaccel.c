/******************************************************
 *    Filename:     gfxaccel.c
 *     Purpose:     graphics accelerator driver
 *  Created on: 	2021/01/18
 * Modified on:
 *      Author: 	atsupi.com
 *     Version:		0.80
 ******************************************************/

//#define DEBUG

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "azplf_bsp.h"
#include "gfxaccel.h"

static u32 GfxaccelAddress = 0;

u32 gfxaccel_read_reg(u32 adr, u32 offset)
{
	if (!adr)
	{
		printf("gfxaccel_read_reg baseaddress error!\n");
		return 0;
	}
	return (*(volatile unsigned int *)(adr + offset));
}

void gfxaccel_write_reg(u32 adr, u32 offset, u32 value)
{
	if (!adr)
	{
		printf("gfxaccel_write_reg baseaddress error!\n");
		return;
	}
	*(volatile unsigned int *)(adr + offset) = value;
#ifdef DEBUG
	u32 data = gfxaccel_read_reg(adr, offset);
	printf("gfxaccel_read_reg(offset:0x%04x) %x : readreg %x\n", offset, value, data);
#endif
}

u32 gfxaccel_init(void)
{
	int fd;
	u32 result;
	u32 baseAddr = GFXACCEL_BASE_ADDR;
	u32 page_size;

	printf("In gfxaccel_init()\n");
	page_size = sysconf(_SC_PAGESIZE);
	printf("File page size=0x%08x (%dKB)\n", page_size, page_size>>10);

	fd = open("/dev/mem", O_RDWR);
	result = (u32)mmap(NULL, page_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, baseAddr);
	printf("Mapping I/O: 0x%08x to vmem: 0x%08x\n", baseAddr, result);
	close(fd);

	if (result == 0 || result == 0xFFFFFFFF)
	{
		printf("Error: Mapping failure\n");
		return PST_FAILURE;
	}

	GfxaccelAddress = result;
	return (result);
}

void gfxaccel_start(void)
{
    u32 Data;

    Data = gfxaccel_read_reg(GfxaccelAddress, GFXACCEL_CONTROL_ADDR_AP_CTRL) & 0x80;
    gfxaccel_write_reg(GfxaccelAddress, GFXACCEL_CONTROL_ADDR_AP_CTRL, Data | 0x01);
}

void gfxaccel_stop(void)
{
    u32 Data;

    Data = gfxaccel_read_reg(GfxaccelAddress, GFXACCEL_CONTROL_ADDR_AP_CTRL) & 0x80;
    gfxaccel_write_reg(GfxaccelAddress, GFXACCEL_CONTROL_ADDR_AP_CTRL, Data);
}

u32 gfxaccel_isdone(void)
{
    u32 Data;

    Data = gfxaccel_read_reg(GfxaccelAddress, GFXACCEL_CONTROL_ADDR_AP_CTRL);
    return (Data >> 1) & 0x1;
}

u32 gfxaccel_isidle(void)
{
    u32 Data;

    Data = gfxaccel_read_reg(GfxaccelAddress, GFXACCEL_CONTROL_ADDR_AP_CTRL);
    return (Data >> 2) & 0x1;
}

u32 gfxaccel_isready(void)
{
    u32 Data;

    Data = gfxaccel_read_reg(GfxaccelAddress, GFXACCEL_CONTROL_ADDR_AP_CTRL);
    // check ap_start to see if the pcore is ready for next input
    return !(Data & 0x1);
}

static void gfxaccel_set_src_fb(u32 Data)
{
    gfxaccel_write_reg(GfxaccelAddress, GFXACCEL_CONTROL_ADDR_SRC_FB_DATA, Data);
}

static void gfxaccel_set_x1(u32 Data)
{
    gfxaccel_write_reg(GfxaccelAddress, GFXACCEL_CONTROL_ADDR_X1_DATA, Data);
}

static void gfxaccel_set_y1(u32 Data)
{
    gfxaccel_write_reg(GfxaccelAddress, GFXACCEL_CONTROL_ADDR_Y1_DATA, Data);
}

static void gfxaccel_set_dx(u32 Data)
{
    gfxaccel_write_reg(GfxaccelAddress, GFXACCEL_CONTROL_ADDR_DX_DATA, Data);
}

static void gfxaccel_set_dy(u32 Data)
{
    gfxaccel_write_reg(GfxaccelAddress, GFXACCEL_CONTROL_ADDR_DY_DATA, Data);
}

static void gfxaccel_set_dst_fb(u32 Data)
{
    gfxaccel_write_reg(GfxaccelAddress, GFXACCEL_CONTROL_ADDR_DST_FB_DATA, Data);
}

static void gfxaccel_set_x2(u32 Data)
{
    gfxaccel_write_reg(GfxaccelAddress, GFXACCEL_CONTROL_ADDR_X2_DATA, Data);
}

static void gfxaccel_set_y2(u32 Data)
{
    gfxaccel_write_reg(GfxaccelAddress, GFXACCEL_CONTROL_ADDR_Y2_DATA, Data);
}

static void gfxaccel_set_col(u32 Data)
{
    gfxaccel_write_reg(GfxaccelAddress, GFXACCEL_CONTROL_ADDR_COL_DATA, Data);
}

static void gfxaccel_set_mode(u32 Data)
{
    gfxaccel_write_reg(GfxaccelAddress, GFXACCEL_CONTROL_ADDR_MODE_DATA, Data);
}

static void gfxaccel_set_op(u32 Data)
{
    gfxaccel_write_reg(GfxaccelAddress, GFXACCEL_CONTROL_ADDR_OP_DATA, Data);
}

static void HwIpGfxaccel(u32 src_fb, u16 x1, u16 y1, u16 dx, u16 dy, u32 dst_fb, u16 x2, u16 y2, u32 col, u8 mode, u8 op)
{
	gfxaccel_set_src_fb(src_fb);
	gfxaccel_set_dst_fb(dst_fb);
	gfxaccel_set_mode(mode); // mode
	gfxaccel_set_op(op); // op
	gfxaccel_set_col(col); // col
	gfxaccel_set_x1(x1); // x1
	gfxaccel_set_y1(y1); // y1
	gfxaccel_set_dx(dx); // dx
	gfxaccel_set_dy(dy); // dy
	gfxaccel_set_x2(x2); // x2
	gfxaccel_set_y2(y2); // y2

    // Invoke accelerator
#ifdef DEBUG
    printf("Wait for Idle signal...");
#endif
    while (!gfxaccel_isidle());
#ifdef DEBUG
    printf("Done.\r\n\r\n");
    printf("Send start Gfxaccel IP signal\r\n");
#endif
    gfxaccel_start();
#ifdef DEBUG
    printf("Wait for Ready signal...");
#endif
    while (!gfxaccel_isready());
    gfxaccel_stop();
#ifdef DEBUG
    printf("Done.\r\n\r\n");
#endif
}

void gfxaccel_fill_rect(u32 fb, u16 x1, u16 y1, u16 x2, u16 y2, u32 col)
{
	HwIpGfxaccel(0, x1, y1, 0, 0, fb, x2, y2, col, GFXACCEL_MODE_FILLRECT, 0);
}

void gfxaccel_draw_line(u32 fb, u16 x1, u16 y1, u16 x2, u16 y2, u32 col)
{
	HwIpGfxaccel(0, x1, y1, 0, 0, fb, x2, y2, col, GFXACCEL_MODE_LINE, 0);
}

void gfxaccel_bitblt(u32 src_fb, u16 x1, u16 y1, u16 dx, u16 dy, u32 dst_fb, u16 x2, u16 y2, u8 op)
{
	HwIpGfxaccel(src_fb, x1, y1, dx, dy, dst_fb, x2, y2, 0, GFXACCEL_MODE_BITBLT, op);
}
