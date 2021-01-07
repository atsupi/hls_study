/*
 * hls_study_007/
 *  source/top.cpp
 *  testbench/top_tb.cpp
 *
 *  purpose: test AXI4 Master interface to fill frame buffer for GUI
 *  option: color and rectangle position
 *     date: 2021/01/06
 *   author: atsupi.com
 */
#include "ap_int.h"

//#define WIDTH		16
//#define HEIGHT		16
#define WIDTH		800
#define HEIGHT		480
#define FB_SIZE		WIDTH*HEIGHT

void top(ap_uint<32> *fb,
		ap_uint<32> col,
		ap_uint<16> x1, ap_uint<16> y1,
		ap_uint<16> x2, ap_uint<16> y2)
{
//#pragma HLS INTERFACE m_axi port=fb depth=256
#pragma HLS INTERFACE m_axi port=fb depth=368000
#pragma HLS INTERFACE s_axilite port=return
#pragma HLS INTERFACE s_axilite port=col
#pragma HLS INTERFACE s_axilite port=x1
#pragma HLS INTERFACE s_axilite port=y1
#pragma HLS INTERFACE s_axilite port=x2
#pragma HLS INTERFACE s_axilite port=y2

	unsigned int i;
	unsigned int x = 0;
	unsigned int y = 0;

	for (i = 0; i < FB_SIZE; i++)
	{
#pragma HLS loop_tripcount min=256 max=256

		if (x >= x1 && x <= x2 && y >= y1 && y <= y2)
			*fb = col;
		else
			*fb = *fb;
		fb++;

		if (x == WIDTH - 1)
		{
			x = 0;
			y++;
		}
		else
		{
			x++;
		}
	}
}
