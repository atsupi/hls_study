/******************************************************
 *    Filename:     lq070out.c
 *     Purpose:     LQ070 LCD display driver
 *  Created on: 	2021/01/20
 * Modified on:
 *      Author: 	atsupi.com
 *     Version:		1.00
 ******************************************************/

//#define DEBUG

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "azplf_bsp.h"
#include "lq070out.h"

// LQ070 LCD display module control IP
static u32 Lq070OutAddress = 0;

void lq070out_init(void)
{
	int fd;
	u32 result;
	u32 baseAddr = LQ070OUT_BASE_ADDR;
	u32 page_size;

	page_size = sysconf(_SC_PAGESIZE);

	printf("In lq070out_init()\n");
	page_size = sysconf(_SC_PAGESIZE);
	printf("File page size=0x%08x (%dKB)\n", page_size, page_size>>10);

	fd = open("/dev/mem", O_RDWR);
	result = (u32)mmap(NULL, page_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, baseAddr);
	printf("Mapping I/O: 0x%08x to vmem: 0x%08x\n", baseAddr, result);
	close(fd);

	if (result == 0 || result == 0xFFFFFFFF)
	{
		printf("Error: Mapping failure\n");
		return;
	}

	Lq070OutAddress = result;
}

void lq070out_write_reg(u32 adr, u32 offset, u32 value)
{
	if (!adr)
	{
		printf("lq070out_write_reg baseaddress error!\n");
		return;
	}
	*(volatile unsigned int *)(adr + offset) = value;
}

void lq070out_setmode(int mode)
{
	lq070out_write_reg(Lq070OutAddress, LQ070OUT_MODE_OFFSET, (u32)mode);
}
