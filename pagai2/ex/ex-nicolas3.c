
extern int f(int x);
int main() {
	int i = 0;
	int j = 0;

	while (i < 10) {
		j = 0;
		while (j < 4) {
			i++;
			j++;
		}
		i = i-j+1;
	}
	f(i);
	f(j);
}
