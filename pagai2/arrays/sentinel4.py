from z3 import Reals, Ints, Exists, ForAll, Or, And, Implies, Not
from z3 import Tactic, Then, OrElse, Repeat, ParThen
i,x,n = Ints('i x n')
# i,x,n = Reals('i x n') h2 does not imply h

#qe = Then(Tactic('qe'),Tactic('simplify'),Tactic('propagate-values'),Tactic('propagate-ineqs'))
qe = Tactic('qe')
f = Or(
  And(-i+x==0, -i+n-1==0, i-2>=0),
  And(-n+x+2==0, -i+n-3>=0, i>=0),
  And(x>=0, n-x-3>=0, i-x-1>=0),
  And(-i+x==0, -i+n-2==0, i-1>=0),
  And(-n+x+1==0, -i+n-2>=0, n-3>=0, i>=0),
  And(-i+x-1>=0, n-x-3>=0, i>=0),
  And(-n+x+1==0, n-3>=0, i-n>=0),
  And(-i+x==0, -i+n-3>=0, i>=0))
g = And(n>2, ForAll(x, Implies(And(x>=0, x<n), f)))
h = apply(And,qe(g)[0])

h2 = And(n>2, i>=0, i<=n-2)
#h2neg = Or(n<=2, i<=-1, i>=n-1)

print qe(ForAll(x,Implies(h,h2)))
print qe(ForAll(x,Implies(h2,h)))

from z3 import solve
solve(And(h,Not(h2)))
solve(And(h2,Not(h)))

simplify=Repeat(Then('nnf','ctx-solver-simplify'))
h3 = apply(And,simplify(h)[0])

simplify = Then(Tactic('simplify'), Tactic('propagate-values'), ParThen(Repeat(OrElse(Tactic('split-clause'), Tactic('skip'))), Tactic('propagate-ineqs')))

#print tactic(ForAll(x,Or(h2neg,h)))
