#include "ap_int.h"

#define WIDTH			800
#define HEIGHT			480
#define FB_SIZE			368000
//#define WIDTH			64
//#define HEIGHT			64
//#define FB_SIZE			4096
#define RADIUS			(HEIGHT / 3)

#define MODE_FILLRECT	1
#define MODE_LINE		2
#define MODE_BITBLT		3

#define RGB1(r, g, b)  (((((r) & 1) * 0x3ff << 20) | ((g) & 1) * 0x3ff << 10) | (((b) & 1) * 0x3ff))

extern void GfxAccel(ap_uint<32> *src_fb, ap_uint<16> x1, ap_uint<16> y1, ap_uint<16> dx, ap_uint<16> dy, ap_uint<32> *dst_fb, ap_uint<16> x2, ap_uint<16> y2, ap_uint<32> col, ap_uint<8> mode);

int main()
{
	unsigned int src_frame[FB_SIZE];
	unsigned int dst_frame[FB_SIZE];

	int i, j;
	int dx = 16;
	int dy = 16;
	int ox;
	int oy;
	unsigned int data;
	FILE *fp;

	unsigned int col = (0 << 20) | (64 << 10) | (255);
	short x1 = 4;
	short y1 = 4;
	short x2 = 11;
	short y2 = 11;
	int x, y;

	/*
	 * Fill rectangle
	 */
	GfxAccel(NULL, x1, y1, 0, 0, (ap_uint<32> *)dst_frame, x2, y2, col, MODE_FILLRECT);

	// Verify frame buffer
	for(i = 0; i < FB_SIZE; i++)
	{
		fprintf(stdout, "table[%d] = 0x%x\n", i, dst_frame[i]);
		x = i % WIDTH;
		y = i / WIDTH;
		if ((x >= x1 && x <= x2 && y >= y1 && y <= y2) &&
			dst_frame[i] != col)
			return 1;
	}

	/*
	 * Draw lines
	 */
	int cx = WIDTH / 2;
	int cy = HEIGHT / 2;
	ox = cx + RADIUS; // x position at 0 degree
	oy = cy;          // y position at 0 degree

	for (i = 0; i < 12; i++) {
		float c = cos((i + 1) * 30 * M_PI / 180.0);
		float s = sin((i + 1) * 30 * M_PI / 180.0);
		int x = cx + c * RADIUS;
		int y = cy + s * RADIUS;

		printf("%02d: (%d,%d)-(%d,%d)\n", i, ox, oy, x, y);
		GfxAccel(NULL, ox, oy, 0, 0, (ap_uint<32> *)dst_frame, x, y, 0xffffffff, MODE_LINE);
		ox = x;
		oy = y;
	}

	fp = fopen("sample_line.ppm", "wb");
	// Write ppm header
	fprintf(fp, "P3 %d %d 255\n", WIDTH, HEIGHT);
	// Write body
	for (i = 0; i < FB_SIZE; i++) {
		fprintf(fp, "%d %d %d\n",
				(dst_frame[i] >> 16) & 0xff,
				(dst_frame[i] >>  8) & 0xff,
				(dst_frame[i] >>  0) & 0xff);
	}
	fclose(fp);

	/*
	 * BitBlt
	 */
	// Prepare resource frame buffer for BitBlt
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
		GfxAccel((ap_uint<32> *)src_frame, ox, oy, dx, dy, (ap_uint<32> *)dst_frame, ox, oy, 0, MODE_BITBLT);
	}

	fp = fopen("sample_bitblt.ppm", "w");
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
