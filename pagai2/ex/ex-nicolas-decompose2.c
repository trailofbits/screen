int main() {
	int i = 0;
	int j = 0;

	while (i < 4) {
		j = 0;
		assert(i = i+4);
		assert(j = 4);
		i = i-j+1;
	}

	assert(i == 4);
}
