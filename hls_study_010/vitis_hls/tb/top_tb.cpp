/*
 * Filename: top.cpp
 * Purpose: to write bresenham line algorithm running on Vitis HLS
 *
 * Date: 2021/01/09
 * Author: atsupi.com
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "ap_int.h"

#define WIDTH			800
#define HEIGHT			480
#define FB_SIZE			WIDTH*HEIGHT
#define RADIUS			160

extern void DrawLine(ap_uint<32> *fb, ap_uint<16> x1, ap_uint<16> y1, ap_uint<16> x2, ap_uint<16> y2, ap_uint<32> col);

int main()
{
	static unsigned int frame_buffer[FB_SIZE];

	int i, j;
	int cx = WIDTH / 2;
	int cy = HEIGHT / 2;
	int ox = cx + RADIUS;
	int oy = cy;
	FILE *fp;

	for (i = 0; i < 12; i++) {
		float c = cos((i + 1) * 30 * M_PI / 180.0);
		float s = sin((i + 1) * 30 * M_PI / 180.0);
		int x = cx + c * RADIUS;
		int y = cy + s * RADIUS;

		printf("%02d: (%d,%d)-(%d,%d)\n", i, ox, oy, x, y);
		DrawLine((ap_uint<32> *)frame_buffer, ox, oy, x, y, 0xffffffff);
		ox = x;
		oy = y;
	}

	fp = fopen("sample.ppm", "wb");
	// Write ppm header
	fprintf(fp, "P6\n");
	fprintf(fp, "%d %d\n", WIDTH, HEIGHT);
	fprintf(fp, "255\n");
	// Write body
	for (i = 0; i < FB_SIZE; i++) {
		fprintf(fp, "%c%c%c",
				(frame_buffer[i] >> 16) & 0xff,
				(frame_buffer[i] >>  8) & 0xff,
				(frame_buffer[i] >>  0) & 0xff);
	}
	fclose(fp);
	return 0;
}





