
#include "ap_int.h"

#define WIDTH		8
#define HEIGHT		8
#define FB_SIZE		WIDTH*HEIGHT

void top(ap_uint<32> *fb)
{
//#pragma HLS INTERFACE m_axi port=fb depth=480000
#pragma HLS INTERFACE m_axi port=fb depth=64 offset=slave
#pragma HLS INTERFACE s_axilite port=return

	unsigned int i;
	unsigned int x, y;
	unsigned int *addr;

	for (i = 0; i < FB_SIZE; i++)
	{
//#pragma HLS loop_tripcount min=64 max=480000
#pragma HLS loop_tripcount min=64 max=64
//		addr = fb + y * WIDTH + x;
		x = i % 8;
		y = i / 8;
		if (x == 0 || y == 0 || x == (WIDTH-1) || y == (HEIGHT-1))
			*fb = 0xffffffff;
		else
			*fb = 0;
		fb++;
	}
}
