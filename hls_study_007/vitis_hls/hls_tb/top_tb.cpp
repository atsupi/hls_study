
#include "string.h"
#include "ap_int.h"

//#define WIDTH		16
//#define HEIGHT		16
#define WIDTH		800
#define HEIGHT		480
#define FB_SIZE		WIDTH*HEIGHT

extern void top(ap_uint<32> *fb,
		ap_uint<32> col,
		ap_uint<16> x1, ap_uint<16> y1,
		ap_uint<16> x2, ap_uint<16> y2);

int main(void)
{
	using namespace std;

	unsigned int table[FB_SIZE];
	for(int i = 0; i < FB_SIZE; i++)
		table[i] = 0;

	unsigned int col = (0 << 20) | (64 << 10) | (255);
	unsigned short x1 = 4;
	unsigned short y1 = 4;
	unsigned short x2 = 11;
	unsigned short y2 = 11;
	int x, y;

	top((ap_uint<32> *)table, col, x1, y1, x2, y2);

	for(int i = 0; i < FB_SIZE; i++)
	{
		cout << "table[" << i << "]=" << table[i] << endl;
		x = i % WIDTH;
		y = i / WIDTH;
		if ((x >= x1 && x <= x2 && y >= y1 && y <= y2) &&
			table[i] != col)
			return 1;
	}

	return 0;
}
