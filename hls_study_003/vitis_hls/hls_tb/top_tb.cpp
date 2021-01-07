// top.cpp
// 2021/01/03 atsupi.com
// Produced with Vitis HLS 2020.2
// Purpose: to study Vitis HLS interface classes

#include "string.h"
#include "ap_int.h"

extern ap_int<16> top(ap_int<8> x, ap_int<8> a, ap_int<8> b, ap_int<8> c);

int main(void)
{
	using namespace std;
	ap_int<16> result;

	for (ap_int<8> i = 0; i < 5; i++)
	{
		result = top((ap_int<8>)i+3, (ap_int<8>)i+2, (ap_int<8>)i+1, (ap_int<8>)i);
		cout << "top(" << i << ") = " << result << endl;
		if (result != ((i+3)*(i+2)+(i+1)+i))
			return 1;
	}
	return 0;
}


