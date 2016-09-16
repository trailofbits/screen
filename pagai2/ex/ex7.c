

int main(int argc, char ** argv) {

	int a=0,b=0,c=0,d=0,e=0,f=0,g=0,h=0;
	int x=0,y=0,z=0;
	int i=0,j=0;
	
	x = 0;
	y = 1000;
	a = 100;
	b = 200;
	d = 5;

	while (x < y) {
	
		for (i = 0; i < a; i++) {
			for (j = 0; j < b; j++) {
				if (j+i < 150) {
					z = 1;
					d += 3;
				} else {
					z = -1;
					d--;
				}
				x = x + z;
				y = y - z;
			}
		}
		
	}

	for (i = 0; i > c; i++) {
		c = c + d;
		while (y > 0) {
			y--;
			e = e+x+y;
			if (e < 0) {
				x--;
				d--;
			} else {
				x++;
				y++;
			}
		}
	}

	return 1;
}
