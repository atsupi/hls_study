/*
 * Filename: top_accel.cpp
 *  Purpose: Graphics acceleration algorithms running on Vitis HLS
 *           (Fill rectangle / Draw line / Bit block transfer)
 *      Ver: 1.20
 *     Date: 2021/01/15
 *   Author: atsupi.com
 */

#include "ap_int.h"

#define WIDTH			800
#define HEIGHT			480
#define FB_SIZE			368000
//#define WIDTH			64
//#define HEIGHT			64
//#define FB_SIZE			4096

#define MODE_FILLRECT	1
#define MODE_LINE		2
#define MODE_BITBLT		3

#define BB_OP_NONE		0
#define BB_OP_OR		1
#define BB_OP_AND		2
#define BB_OP_XOR		3

void GfxAccel(ap_uint<32> *src_fb, ap_uint<16> x1, ap_uint<16> y1, ap_uint<16> dx, ap_uint<16> dy, ap_uint<32> *dst_fb, ap_uint<16> x2, ap_uint<16> y2, ap_uint<32> col, ap_uint<8> mode, ap_uint<8> op)
{
//#pragma HLS INTERFACE m_axi port=src_fb depth=368000 offset=slave
//#pragma HLS INTERFACE m_axi port=dst_fb depth=368000 offset=slave
#pragma HLS INTERFACE m_axi port=src_fb depth=4096 offset=slave
#pragma HLS INTERFACE m_axi port=dst_fb depth=4096 offset=slave
#pragma HLS INTERFACE s_axilite port=return
#pragma HLS INTERFACE s_axilite port=x1
#pragma HLS INTERFACE s_axilite port=y1
#pragma HLS INTERFACE s_axilite port=dx
#pragma HLS INTERFACE s_axilite port=dy
#pragma HLS INTERFACE s_axilite port=x2
#pragma HLS INTERFACE s_axilite port=y2
#pragma HLS INTERFACE s_axilite port=col
#pragma HLS INTERFACE s_axilite port=mode
#pragma HLS INTERFACE s_axilite port=op

	int i, j;
	unsigned int buf[WIDTH];

	if (mode == MODE_FILLRECT) {
		short mx = (x1 < x2)? (x2 - x1): (x1 - x2);
		short my = (y1 < y2)? (y2 - y1): (y1 - y2);
		mx++;
		my++;
		dst_fb += x1 + y1 * WIDTH;

		for (i = 0; i < my; i++)
		{
	#pragma HLS loop_tripcount min=1 max=480
//#pragma HLS loop_tripcount min=1 max=HEIGHT
			for (j = 0; j < mx; j++) {
	#pragma HLS loop_tripcount min=1 max=800
//#pragma HLS loop_tripcount min=1 max=WIDTH
				*dst_fb = col;
				dst_fb++;
			}
			dst_fb += (WIDTH - mx);
		}
	} else if (mode == MODE_LINE) {
		short mx = (x1 < x2)? (x2 - x1): (x1 - x2);
		short my = (y1 < y2)? (y2 - y1): (y1 - y2);
		short sx = (x1 < x2)? 1: (x1 > x2)? -1: 0;
		short sy = (y1 < y2)? 1: (y1 > y2)? -1: 0;
		short loop = (mx >= my)? mx: my;
		short e = (mx >= my)? -mx: -my;

		dst_fb += x1 + y1 * WIDTH;
		if (mx >= my) {
			for (i = 0; i < loop; i++ ) {
	#pragma HLS loop_tripcount min=1 max=800
//#pragma HLS loop_tripcount min=1 max=WIDTH
				*dst_fb = col;
				dst_fb += sx;
				e += (my << 1); // multiply by 2
				if (e >= 0) {
					dst_fb += sy * WIDTH;
					e -= (mx << 1); // multiply by 2
				}
			}
		} else { // mx < my
			for (i = 0; i < loop; i++ ) {
	#pragma HLS loop_tripcount min=1 max=800
//#pragma HLS loop_tripcount min=1 max=WIDTH
				*dst_fb = col;
				dst_fb += sy * WIDTH;
				e += (mx << 1); // multiply by 2
				if (e >= 0) {
					dst_fb += sx;
					e -= (my << 1); // multiply by 2
				}
			}
		}
	} else if (mode == MODE_BITBLT && op == BB_OP_NONE) {
		src_fb += x1 + y1 * WIDTH;
		dst_fb += x2 + y2 * WIDTH;

		for (i = 0; i < dy; i++)
		{
	#pragma HLS loop_tripcount min=1 max=480
//#pragma HLS loop_tripcount min=1 max=HEIGHT
			for (j = 0; j < dx; j++) {
	#pragma HLS loop_tripcount min=1 max=800
//#pragma HLS loop_tripcount min=1 max=WIDTH
				buf[j] = *src_fb;
				src_fb++;
			}
			for (j = 0; j < dx; j++) {
	#pragma HLS loop_tripcount min=1 max=800
//#pragma HLS loop_tripcount min=1 max=WIDTH
				*dst_fb = buf[j];
				dst_fb++;
			}
			src_fb += (WIDTH - dx);
			dst_fb += (WIDTH - dx);
		}
	} else if (mode == MODE_BITBLT) {
		src_fb += x1 + y1 * WIDTH;
		dst_fb += x2 + y2 * WIDTH;

		for (i = 0; i < dy; i++)
		{
	#pragma HLS loop_tripcount min=1 max=480
//#pragma HLS loop_tripcount min=1 max=HEIGHT
			for (j = 0; j < dx; j++) {
	#pragma HLS loop_tripcount min=1 max=800
//#pragma HLS loop_tripcount min=1 max=WIDTH
				switch (op) {
				case BB_OP_OR:
					buf[j] = *dst_fb | *src_fb;
					break;
				case BB_OP_AND:
					buf[j] = *dst_fb & *src_fb;
					break;
				case BB_OP_XOR:
					buf[j] = *dst_fb ^ *src_fb;
					break;
				default:
					buf[j] = *src_fb;
					break;
				}
				src_fb++;
				dst_fb++;
			}
			dst_fb -= dx;
			for (j = 0; j < dx; j++) {
	#pragma HLS loop_tripcount min=1 max=800
//#pragma HLS loop_tripcount min=1 max=WIDTH
				*dst_fb = buf[j];
				dst_fb++;
			}
			src_fb += (WIDTH - dx);
			dst_fb += (WIDTH - dx);
		}
	}
}
