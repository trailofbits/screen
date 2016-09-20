

int recursive_sum(int x) {
	if (x <= 0) return 0;
	else return x + recursive_sum(x-1);
}
