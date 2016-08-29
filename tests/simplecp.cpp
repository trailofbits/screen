#include <iostream>
 
int main()
{
 int a = 5;
 int b = 100 + a;
 if (a==5) {
	 b = 2;
 } else {
	 b = 10;
 }
 int c = b*50;
 int d = c/25;
 int d2 = d%25;
 int e = d2+100;
 int f = e-100;
 int g = f >> 2;
 int h = g << 2;
 int i = h & a;
 int j = i | a;
 return j;
}
