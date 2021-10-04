#include "stl_vector.h"
using namespace yyt_stl;
int main() {
	vector<int> v2(10);
	printf("%d", v2.capacity());
	for (int i = 0; i < 10; i++)
	{
		v2.push_back(i);
	}
	return 0;
}