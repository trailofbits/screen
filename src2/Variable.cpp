/*
 * Variable.cpp
 *
 *  Created on: 2014-05-23
 *      Author: jtestard
 */
#include "Variable.h"

Variable::Variable(string name) {
	this->name = name;
}

Variable::~Variable() {

}

string Variable::GetName(){
	return this->name;
}
