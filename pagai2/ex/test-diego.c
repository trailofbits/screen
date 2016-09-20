#define _true 1

typedef struct {
   int x;
} St;

int foo(St *t){
   int y, z, w, a, b;
   if (t->x) 
      y = z;
   else 
      y = 0;
   y++;
   if (a)
      w = y;
   else
      w = z;
   w++;
   if (t->x) 
      z = 1-y;
   else 
      z = 0;
   z++;
   if (b)
      z = y;
   else
      z = y+1;
   z++;
   if (t->x) 
      w = z-y;
   else 
      w = y-z-1;
   return w;
}
