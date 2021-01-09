/*
 * Filename: top.cpp
 * Purpose: to write bresenham line algorithm running on Vitis HLS
 *
 * Date: 2021/01/09
 * Author: atsupi.com
 */

#include "ap_int.h"

#define WIDTH			800
#define HEIGHT			480
#define FB_SIZE			WIDTH*HEIGHT

void DrawLine(ap_uint<32> *fb, ap_uint<16> x1, ap_uint<16> y1, ap_uint<16> x2, ap_uint<16> y2, ap_uint<32> col)
{
#pragma HLS INTERFACE m_axi port=fb depth=368000 offset=slave
#pragma HLS INTERFACE s_axilite port=return
#pragma HLS INTERFACE s_axilite port=x1
#pragma HLS INTERFACE s_axilite port=y1
#pragma HLS INTERFACE s_axilite port=x2
#pragma HLS INTERFACE s_axilite port=y2
#pragma HLS INTERFACE s_axilite port=col
	short dx = (x1 < x2)? (x2 - x1): (x1 - x2);
	short dy = (y1 < y2)? (y2 - y1): (y1 - y2);
	short sx = (x1 < x2)? 1: (x1 > x2)? -1: 0;
	short sy = (y1 < y2)? 1: (y1 > y2)? -1: 0;
	short loop = (dx >= dy)? dx: dy;
	short e = (dx >= dy)? -dx: -dy;
	int i;

	fb += x1 + y1 * WIDTH;
	if (dx >= dy) {
		for (i = 0; i < loop; i++ ) {
#pragma HLS loop_tripcount min=1 max=800
			*fb = col;
			fb += sx;
			e += (dy << 1); // multiply by 2
			if (e >= 0) {
				fb += sy * WIDTH;
				e -= (dx << 1); // multiply by 2
			}
		}
	} else { // dx < dy
		for (i = 0; i < loop; i++ ) {
#pragma HLS loop_tripcount min=1 max=800
			*fb = col;
			fb += sy * WIDTH;
			e += (dx << 1); // multiply by 2
			if (e >= 0) {
				fb += sx;
				e -= (dy << 1); // multiply by 2
			}
		}
	}
}
