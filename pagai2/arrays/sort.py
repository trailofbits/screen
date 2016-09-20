from z3 import Bools, Reals, Ints, Exists, ForAll, Or, And, Implies, Not
from z3 import Tactic, Then, OrElse, Repeat, ParThen

n,i,k,x,y,z,a,b,c = Ints('n i k x y z a b c');
x0l,x0r = Bools('x0l x0r');
x1l,x1r = Bools('x1l x1r');
x2l,x2r = Bools('x2l x2r');
y0l,y0r = Bools('y0l y0r');
y1l,y1r = Bools('y1l y1r');
y2l,y2r = Bools('y2l y2r');
z0l,z0r = Bools('z0l z0r');
z1l,z1r = Bools('z1l z1r');
z2l,z2r = Bools('z2l z2r');

inv = [
  (And(x0l, x0r, x2l, x2r, Not(y0l), y0r, Not(y2l), y2r, Not(z0l), z0r, Not(z2l), z2r),
   And(-i+x+1==0, -i+k+1==0, -a+c>=0, -a+b>=0, -i+y>=0, -y+z-1>=0,
    n-z-1>=0, i-1>=0)),
  (And(x0l, x0r, x2l, Not(x2r), Not(y0l), y0r, y2l, y2r, Not(z0l), z0r, Not(z2l), z2r),
   And(-i+x+1==0, -k+y==0, -a+c>=0, -a+b-1>=0, -i+k>=0, -k+z-1>=0, n-z-1>=0, i-1>=0)),
  (And(x0l, x0r, x2l, Not(x2r), Not(y0l), y0r, y2l, Not(y2r), Not(z0l), z0r, z2l, z2r),
   And(-i+x+1==0, -k+z==0, -a+c-1>=0, -a+b-1>=0, -i+y>=0, -k+n-1>=0, k-y-1>=0, i-1>=0)),
  (And(x0l, x0r, x2l, Not(x2r), Not(y0l), y0r, y2l, Not(y2r), Not(z0l), z0r, z2l, Not(z2r)),
   And(-i+x+1==0, -a+c-1>=0, -a+b-1>=0, -i+y>=0, -k+n-1>=0, -y+z-1>=0, k-z-1>=0, i-1>=0)),
  (And(x0l, x0r, x2l, Not(x2r), Not(y0l), y0r, y2l, Not(y2r), Not(z0l), z0r, Not(z2l), z2r),
   And(-i+x+1==0, -a+c>=0, -a+b-1>=0, -i+y>=0, -k+z-1>=0, n-z-1>=0, k-y-1>=0, i-1>=0)),
  (And(x0l, x0r, x2l, Not(x2r), Not(y0l), y0r, Not(y2l), y2r, Not(z0l), z0r, Not(z2l), z2r),
   And(-i+x+1==0, -a+c>=0, -a+b>=0, -i+k>=0, -k+y-1>=0, -y+z-1>=0, n-z-1>=0, i-1>=0)),
  (And(x0l, Not(x0r), x2l, Not(x2r), y0l, y0r, y2l, y2r, Not(z0l), z0r, Not(z2l), z2r),
   And(-i+y+1==0, -i+k+1==0, -b+c>=0, -i+z>=0, x>=0, n-z-1>=0, i-x-2>=0)),
  (And(x0l, Not(x0r), x2l, Not(x2r), y0l, y0r, y2l, Not(y2r), Not(z0l), z0r, z2l, z2r),
   And(-i+y+1==0, -k+z==0, -b+c-1>=0, -i+k>=0, -k+n-1>=0, x>=0, i-x-2>=0)),
  (And(x0l, Not(x0r), x2l, Not(x2r), y0l, y0r, y2l, Not(y2r), Not(z0l), z0r, z2l, Not(z2r)),
   And(-i+y+1==0, -b+c-1>=0, -i+z>=0, -k+n-1>=0, x>=0, k-z-1>=0, i-x-2>=0)),
  (And(x0l, Not(x0r), x2l, Not(x2r), y0l, y0r, y2l, Not(y2r), Not(z0l), z0r, Not(z2l), z2r),
   And(-i+y+1==0, -b+c>=0, -i+k>=0, -k+z-1>=0, x>=0, n-z-1>=0, i-x-2>=0)),
  (And(x0l, Not(x0r), x2l, Not(x2r), y0l, Not(y0r), y2l, Not(y2r), z0l, z0r, z2l, z2r),
   And(-i+z+1==0, -i+k+1==0, -i+n-1>=0, -x+y-1>=0, x>=0, i-y-2>=0)),
  (And(x0l, Not(x0r), x2l, Not(x2r), y0l, Not(y0r), y2l, Not(y2r), z0l, z0r, z2l, Not(z2r)),
   And(-i+z+1==0, -i+k>=0, -k+n-1>=0, -x+y-1>=0, x>=0, i-y-2>=0)),
  (And(x0l, Not(x0r), x2l, Not(x2r), y0l, Not(y0r), y2l, Not(y2r), z0l, Not(z0r), z2l, Not(z2r)),
   And(-i+n-1>=0, -i+k+1>=0, -k+n-1>=0, -x+y-1>=0, -y+z-1>=0, x>=0, i-z-2>=0)),
  (And(x0l, Not(x0r), x2l, Not(x2r), y0l, Not(y0r), y2l, Not(y2r), Not(z0l), z0r, z2l, z2r),
   And(-k+z==0, -i+k>=0, -k+n-1>=0, -x+y-1>=0, x>=0, i-y-2>=0)),
  (And(x0l, Not(x0r), x2l, Not(x2r), y0l, Not(y0r), y2l, Not(y2r), Not(z0l), z0r, z2l, Not(z2r)),
   And(-i+z>=0, -k+n-1>=0, -x+y-1>=0, x>=0, k-z-1>=0, i-y-2>=0)),
  (And(x0l, Not(x0r), x2l, Not(x2r), y0l, Not(y0r), y2l, Not(y2r), Not(z0l), z0r, Not(z2l), z2r),
   And(-i+k+1>=0, -k+z-1>=0, -x+y-1>=0, x>=0, n-z-1>=0, i-y-2>=0)),
  (And(x0l, Not(x0r), x2l, Not(x2r), Not(y0l), y0r, y2l, y2r, Not(z0l), z0r, Not(z2l), z2r),
   And(-k+y==0, -i+k>=0, -k+z-1>=0, x>=0, n-z-1>=0, i-x-2>=0)),
  (And(x0l, Not(x0r), x2l, Not(x2r), Not(y0l), y0r, y2l, Not(y2r), Not(z0l), z0r, z2l, z2r),
   And(-k+z==0, -i+y>=0, -k+n-1>=0, x>=0, k-y-1>=0, i-x-2>=0)),
  (And(x0l, Not(x0r), x2l, Not(x2r), Not(y0l), y0r, y2l, Not(y2r), Not(z0l), z0r, z2l, Not(z2r)),
   And(-i+y>=0, -k+n-1>=0, -y+z-1>=0, x>=0, k-z-1>=0, i-x-2>=0)),
  (And(x0l, Not(x0r), x2l, Not(x2r), Not(y0l), y0r, y2l, Not(y2r), Not(z0l), z0r, Not(z2l), z2r),
   And(-i+y>=0, -k+z-1>=0, x>=0, n-z-1>=0, k-y-1>=0, i-x-2>=0)),
  (And(x0l, Not(x0r), x2l, Not(x2r), Not(y0l), y0r, Not(y2l), y2r, Not(z0l), z0r, Not(z2l), z2r),
   And(-i+k+1>=0, -k+y-1>=0, -y+z-1>=0, x>=0, n-z-1>=0, i-x-2>=0)),
  (And(Not(x0l), x0r, x2l, x2r, Not(y0l), y0r, Not(y2l), y2r, Not(z0l), z0r, Not(z2l), z2r),
   And(-k+x==0, -i+k>=0, -k+y-1>=0, -y+z-1>=0, n-z-1>=0, i-1>=0)),
  (And(Not(x0l), x0r, x2l, Not(x2r), Not(y0l), y0r, y2l, y2r, Not(z0l), z0r, Not(z2l), z2r),
   And(-k+y==0, -i+x>=0, -k+z-1>=0, n-z-1>=0, k-x-1>=0, i-1>=0)),
  (And(Not(x0l), x0r, x2l, Not(x2r), Not(y0l), y0r, y2l, Not(y2r), Not(z0l), z0r, z2l, z2r),
   And(-k+z==0, -i+x>=0, -k+n-1>=0, -x+y-1>=0, k-y-1>=0, i-1>=0)),
  (And(Not(x0l), x0r, x2l, Not(x2r), Not(y0l), y0r, y2l, Not(y2r), Not(z0l), z0r, z2l, Not(z2r)),
   And(-i+x>=0, -k+n-1>=0, -x+y-1>=0, -y+z-1>=0, k-z-1>=0, i-1>=0)),
  (And(Not(x0l), x0r, x2l, Not(x2r), Not(y0l), y0r, y2l, Not(y2r), Not(z0l), z0r, Not(z2l), z2r),
   And(-i+x>=0, -k+z-1>=0, -x+y-1>=0, n-z-1>=0, k-y-1>=0, i-1>=0)),
  (And(Not(x0l), x0r, x2l, Not(x2r), Not(y0l), y0r, Not(y2l), y2r, Not(z0l), z0r, Not(z2l), z2r),
   And(-i+x>=0, -k+y-1>=0, -y+z-1>=0, n-z-1>=0, k-x-1>=0, i-1>=0)),
  (And(Not(x0l), x0r, Not(x2l), x2r, Not(y0l), y0r, Not(y2l), y2r, Not(z0l), z0r, Not(z2l), z2r),
   And(-i+k+1>=0, -k+x-1>=0, -x+y-1>=0, -y+z-1>=0, n-z-1>=0, i-1>=0)),
  (And(Not(x0l), Not(x0r), Not(x2l), Not(x2r), Not(y0l), Not(y0r), Not(y2l), Not(y2r), Not(z0l), Not(z0r), Not(z2l), Not(z2r)),
   And(i==0, -x+y-1>=0, -y+z-1>=0, x>=0, n-z-1>=0))];

qe = Tactic('qe')
simplify=Repeat(Then('nnf','ctx-solver-simplify'))
def simpl(f):
  return apply(And, Then(qe, simplify)(f)[0])

