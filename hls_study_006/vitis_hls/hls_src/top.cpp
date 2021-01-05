/*
 * hls_study_006/
 *  source/top.cpp
 *  testbench/top_tb.cpp
 *
 *  purpose: test AXI4 Master interface to fill frame buffer for GUI
 *     date: 2021/01/04
 *   author: atsupi.com
 */
#include "ap_int.h"

#define WIDTH		800
#define HEIGHT		480
#define FB_SIZE		WIDTH*HEIGHT

void top(ap_uint<32> *fb)
{
#pragma HLS INTERFACE m_axi port=fb depth=368000
//#pragma HLS INTERFACE m_axi port=fb depth=64
#pragma HLS INTERFACE s_axilite port=return

	unsigned int i;
	unsigned int x, y;

	for (i = 0; i < FB_SIZE; i++)
	{
//#pragma HLS loop_tripcount min=64 max=480000
#pragma HLS loop_tripcount min=64 max=368000
		x = i % 800;
		y = i / 800;
		if (x == 80 || y == 80 || x == (WIDTH-80) || y == (HEIGHT-80))
			*fb = 0xffffffff;
		else
			*fb = 0;
		fb++;
	}
}
