/*
 * Filename: top.cpp
 * Purpose: to write bitblt line algorithm running on Vitis HLS
 *
 * Date: 2021/01/10
 * Author: atsupi.com
 */

#include <string.h>
#include "ap_int.h"

#define WIDTH			800
#define HEIGHT			480
//#define WIDTH			64
//#define HEIGHT			64
#define FB_SIZE			WIDTH*HEIGHT

void BitBlt(ap_uint<32> *src_fb, ap_uint<16> x1, ap_uint<16> y1, ap_uint<16> dx, ap_uint<16> dy, ap_uint<32> *dst_fb, ap_uint<16> x2, ap_uint<16> y2)
{
#pragma HLS INTERFACE m_axi port=src_fb depth=368000 offset=slave
#pragma HLS INTERFACE m_axi port=dst_fb depth=368000 offset=slave
//#pragma HLS INTERFACE m_axi port=src_fb depth=4096 offset=slave
//#pragma HLS INTERFACE m_axi port=dst_fb depth=4096 offset=slave
#pragma HLS INTERFACE s_axilite port=return
#pragma HLS INTERFACE s_axilite port=x1
#pragma HLS INTERFACE s_axilite port=y1
#pragma HLS INTERFACE s_axilite port=dx
#pragma HLS INTERFACE s_axilite port=dy
#pragma HLS INTERFACE s_axilite port=x2
#pragma HLS INTERFACE s_axilite port=y2

	unsigned int buf[WIDTH];
	int i;
	int j;

	src_fb += x1 + y1 * WIDTH;
	dst_fb += x2 + y2 * WIDTH;

	for (i = 0; i < dy; i++)
	{
#pragma HLS loop_tripcount min=1 max=64
		for (j = 0; j < dx; j++) {
#pragma HLS loop_tripcount min=1 max=64
//		memcpy(buf, src_fb, dx * 4);
//		memcpy(dst_fb, buf, dx * 4);
//		memcpy(src_fb, dst_fb, dx * 4);
			buf[j] = *src_fb;
			src_fb++;
		}
		for (j = 0; j < dx; j++) {
#pragma HLS loop_tripcount min=1 max=64
			*dst_fb = buf[j];
			dst_fb++;
		}
		src_fb += (WIDTH - dx);
		dst_fb += (WIDTH - dx);
	}
}
