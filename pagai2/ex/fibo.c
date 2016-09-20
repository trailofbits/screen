

int fibo(int x) {
	if (x <= 0) return 0;
	else if (x <= 1) return x;
	else return fibo(x-1) + fibo(x-2);

}
