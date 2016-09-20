int undef();

int main() {
	int i = 0;
	int j = 0;

	while (i < 4) {
		j = 0;
		i = undef();
		j = undef();
		i = i-j+1;
	}

	assert (i == 4);
}
