#include <vector>

int f() {
	std::vector<int> v;

	v.push_back(0);

	for(int i=0; i < 42; i++) {
		v.push_back(i+v.back());
	}
	return 0;
}
