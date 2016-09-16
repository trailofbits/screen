#include "pagai_assert.h"
int main() {
	
int k=0;
double h=0.01;
double x1=0.2;
double x2=0.3;
double xo1=0.2;
double xo2=0.3;

double a11=0.05;
double a12=-0.5;
double a21=2.0;
double a22=0.05;

double b11=-0.05;
double b12=-2.0;
double b21=0.5;
double b22=0.05;

	loc1:
		while (1) {
				
				assume(x1<=2.0);
				x1= xo1 + (a11*xo1 + a12*xo2)*h;
				x2= xo2 + (a21*xo1 + a22*xo2)*h;
				xo1=x1;
				xo2=x2;
				if (x1>=-0.2 && x1<=-0.1 && x2>=-0.2 && x2<=0.01) { goto loc2; }
			} 
	loc2: 
		while (1) {
				
				assume(x1<=2.0);
				x1= xo1 + (b11*xo1 + b12*xo2)*h;
				x2= xo2 + (b21*xo1 + b22*xo2)*h;
				xo1=x1;
				xo2=x2;
				if (x1>=0.01 && x1<=0.03 && x2>=-0.01 && x2<=0.1) { goto loc1; }
			} 			 
	
	//assert(k<=1000);
}
