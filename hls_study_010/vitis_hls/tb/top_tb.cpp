/*
 * Filename: top.cpp
 * Purpose: to write bitblt algorithm running on Vitis HLS
 *
 * Date: 2021/01/10
 * Author: atsupi.com
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "ap_int.h"

#define WIDTH			800
#define HEIGHT			480
//#define WIDTH			64
//#define HEIGHT			64
#define FB_SIZE			WIDTH*HEIGHT
#define RGB1(r, g, b)  (((((r) & 1) * 0x3ff << 20) | ((g) & 1) * 0x3ff << 10) | (((b) & 1) * 0x3ff))

extern void BitBlt(ap_uint<32> *src_fb, ap_uint<16> x1, ap_uint<16> y1, ap_uint<16> dx, ap_uint<16> dy, ap_uint<32> *dst_fb, ap_uint<16> x2, ap_uint<16> y2);

int main()
{
	static unsigned int src_frame[FB_SIZE];
	static unsigned int dst_frame[FB_SIZE];

	int i, j;
	int dx = 16;
	int dy = 16;
	int ox;
	int oy;
	unsigned int col;
	unsigned int data;
	FILE *fp;

	for (i = 0; i < HEIGHT; i++) {
		for (j = 0; j < WIDTH; j++) {
			col = ((i / 16) << 2) | (j / 16);
			data = RGB1(col >> 2, col >> 1, col);
			src_frame[i*WIDTH+j] = data;
		}
	}
	for (i = 0; i < (WIDTH >> 4) * (HEIGHT >> 4); i++) {
		ox = (i % (WIDTH >> 4)) << 4;
		oy = (i / (WIDTH >> 4)) << 4;
		fprintf(stdout, "%02d: (%d,%d)-(%d,%d)\n", i, ox, oy, dx, dy);
		BitBlt((ap_uint<32> *)src_frame, ox, oy, dx, dy, (ap_uint<32> *)dst_frame, ox, oy);
	}

	fp = fopen("sample.ppm", "w");
	// Write ppm header
	fprintf(fp, "P3 %d %d 255\n", WIDTH, HEIGHT);
	// Write body
	for (i = 0; i < FB_SIZE; i++) {
		fprintf(fp, "%d %d %d\n",
				(dst_frame[i] >> 20) & 0xff,
				(dst_frame[i] >> 10) & 0xff,
				(dst_frame[i] >>  0) & 0xff);
	}
	fclose(fp);
	return 0;
}
