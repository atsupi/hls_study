
#include "string.h"
#include "ap_int.h"

#define WIDTH		800
#define HEIGHT		480
#define FB_SIZE		WIDTH*HEIGHT

extern void top(ap_uint<32> *fb);

int main(void)
{
	using namespace std;

	unsigned int table[FB_SIZE];
	for(int i = 0; i < FB_SIZE; i++)
		table[i] = 0;

	top((ap_uint<32> *)table);

/*	for(int i = 0; i < FB_SIZE; i++)
	{
		cout << "table[" << i << "]=" << table[i] << endl;
		if ((i < 8 || i >= 56 || i % 8 == 0 || i % 8 == 7) &&
			table[i] != 0xffffffff)
			return 1;
	}
*/
	return 0;
}
