
#include <boost/numeric/ublas/vector.hpp>

void toto(unsigned n) {
  boost::numeric::ublas::vector<double> v(n);
  for (unsigned i = 0; i < v.size (); i++) {
    v (i) = i;
  }
}

