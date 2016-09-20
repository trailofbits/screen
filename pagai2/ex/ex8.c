//#include<iostream>

int main() {
	int x;
	int y;
	int d;
	x = 1;
	y = 0;

	while(1) {
	//	std::cout << x << " " << y << "\n";
		if (x > 0) {
		} else if (x < 0) {
		} else {
			if (y > -1) {
			} else if (y < -1) {
			} else {
				x=1;
			}
			if (y > 1) {
			} else if (y < 1) {
			} else {
				x=-1;
			}
		}

		if (y > 0) {
		} else if (y < 0) {
		} else {
			if (x > -1) {
			} else if (x < -1) {
			} else {
				y=-1;
			}
			if (x > 1) {
			} else if (x < 1) {
			} else {
				y=1;
			}
		}


		if (y > 1) {
		} else if (y < 1) {
		} else {
			if (x > 1) {
			} else if (x < 1) {
			} else {
				x=0;
			}
		}

		if (y > -1) {
		} else if (y < -1) {
		} else {
			if (x > -1) {
			} else if (x < -1) {
			} else {
				x=0;
			}
		}
		if (x > 1) {
		} else if (x < 1) {
		} else {
			if (y > -1) {
			} else if (y < -1) {
			} else {
				y=0;
			}
		}
		if (x > -1) {
		} else if (x < -1) {
		} else {
			if (y > 1) {
			} else if (y < 1) {
			} else {
				y=0;
			}
		}
	}
	return 1;
}
