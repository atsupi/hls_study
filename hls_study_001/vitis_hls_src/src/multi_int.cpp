// multi_int.c

#include <ap_int.h>

void multi_int(ap_uint<8>multi_in0, ap_uint<8>multi_in1, ap_uint<16>*multi_out)
{
	*multi_out = multi_in0 * multi_in1;
}
