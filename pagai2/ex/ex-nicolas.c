
int main() {
	int i = 0;
	int j = 0;

	while (i < 4) {
		j = 0;
		while (j < 4) {
			i++;
			j++;
		}
		i = i-j+1;
	}
}
