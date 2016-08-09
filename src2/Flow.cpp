/*
 * Flow.cpp
 *
 *  Created on: 2014-05-29
 *      Author: jtestard
 */


#include "Flow.h"

const string Flow::TOP = "top";
const string Flow::BOTTOM = "bottom";

/**
 * For the basic static analysis, just compare strings.
 */
bool Flow::equals(Flow* other){
	return this->basic==other->basic;
}

string Flow::jsonString(){
	return "\"" + basic + "\"";
}

bool Flow::isBasic() {
	return basic!="";
}

bool Flow::basicEquals(Flow* other){
	return this->basic==other->basic;
}

void Flow::copy(Flow *rhs){
	this->basic = rhs->basic;
}

Flow::Flow(){
	basic = "";
}

Flow::Flow(string input){
	basic = input;
}

Flow::Flow(Flow* flow){
	basic = flow->basic;
}

//Most basic join operation possible.
Flow* Flow::join(Flow* other){
	//join bottom-bottom gives you bottom. Anything else gives you top.
	errs()<< "I just entered into the superclassed join... \n";
	if (this->basic==BOTTOM && other->basic==BOTTOM)
		return new Flow(BOTTOM);
	else
		return new Flow(TOP);
}

Flow::~Flow(){
	//Nothing for basic static analysis
}
