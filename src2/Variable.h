#ifndef VARIABLE
#define VARIABLE
#include <string>

using namespace std;
/**
 * This class is meant to represent variables in the code.
 * This class is not used in practice yet, but could be used in the future.
 *
 */
class Variable {
private :
	string name;
	Variable();
public :
	Variable(string name);
	~Variable();
	string GetName();
};
#endif
