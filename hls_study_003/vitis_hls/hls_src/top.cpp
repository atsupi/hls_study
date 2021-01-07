// top.cpp 
// 2021/01/03 atsupi.com 
// Produced with Vitis HLS 2020.2 
// Purpose: to study Vitis HLS interface classes 

#include "ap_int.h"

ap_int<16> top(ap_int<8> x, ap_int<8> a, ap_int<8> b, ap_int<8> c)
{
	ap_int<16> r;
	r = x * a + b + c;
	return (r);
}
