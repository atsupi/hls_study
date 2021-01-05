// main_tb.c

#include <string.h>
#include <ap_int.h>

void multi_int(ap_uint<8>multi_in0, ap_uint<8>multi_in1, ap_uint<16>*multiout);

int main()
{
	using namespace std;

	ap_uint<8>multi_in0;
	ap_uint<8>multi_in1;
	ap_uint<16>multi_out;
	
	for (multi_in0 = 0; multi_in0 < 10; multi_in0++)
	{
		multi_in1 = multi_in0 + 1;
		multi_int(multi_in0, multi_in1, &multi_out);
//		printf("multi_out = %u\n", multi_out);
		cout << "multi_out = " << multi_out << endl;
		if (multi_out != (multi_in0 * multi_in1))
			return (1);
	}
	
	return (0);
}
