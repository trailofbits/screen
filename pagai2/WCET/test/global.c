

extern int g;
extern int h;

int main() {
	int x = 12;
	int y = 42;
	for (int i = 0; i < 10; i++) {
		x+=g;
		y+=h;
	}
	return x+y;
}
