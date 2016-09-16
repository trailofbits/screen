
int main() {
	int i = 0;
	int j = 0;

	while (i < 10) {
		j = 0;
		while (j < 10) {
			i++;
			j++;
		}
		i = i-j+1;
	}
}
