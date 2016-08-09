/*

 * Flow.cpp
 *
 *  Created on: 2014-05-29
 *      Author: jtestard
 */

#include "RangeAnalysisFlow.h"

/*
 * Flows are equal if their values are equal
 */
bool RangeAnalysisFlow::equals(Flow* otherSuper)
{
	RangeAnalysisFlow* other =
			static_cast<RangeAnalysisFlow*>(otherSuper);
	if (this->isBasic() || other->isBasic())
		return this->basicEquals(other);
	if (other->value.size() != this->value.size())
		return false;
	for (map<string, RangeDomainElement>::const_iterator it = this->value.begin();
			it != this->value.end(); it++) {
		string key = it->first;
		RangeDomainElement thisVal = it->second;
		//Check if key is found in other
		if (other->value.find(key) == other->value.end())
			return false;
		RangeDomainElement otherVal = other->value.find(key)->second;
		if(!RangeDomainElementisEqual(	(const RangeDomainElement*) &otherVal,
										(const RangeDomainElement*) &thisVal)	)
			return false;

	}

	return true;
}

//Represents a constant propagation analysis value as a JSON string.
string RangeAnalysisFlow::jsonString() {
	if (value.size() == 0)
		return "\"" + basic + "\"";
	//Value has something inside
	stringstream ss;
	map<string, RangeDomainElement>::const_iterator it = this->value.begin();
	int counter = 0;
	ss << "{";
	for (; it != this->value.end(); it++) {
		if (counter == 0) {
			ss << "\"" << it->first << "\" : [";
			RangeDomainElement v = it->second;
			ss << v.lower << "," << v.upper << "] ";
		} else {
			ss << ",\"" << it->first << "\" : [";
			RangeDomainElement v = it->second;
			ss << v.lower << "," << v.upper << "] ";
		}

		counter++;

	}
	ss << "}";
	return ss.str();
}

void RangeAnalysisFlow::copy(Flow* rhs) {
	RangeAnalysisFlow* f = static_cast<RangeAnalysisFlow*>(rhs);
	this->basic = f->basic;
	this->value = f->value;
}

RangeAnalysisFlow::RangeAnalysisFlow() :
		Flow() {
}

RangeAnalysisFlow::RangeAnalysisFlow(string input) :
		Flow(input) {
}

RangeAnalysisFlow::RangeAnalysisFlow(
		RangeAnalysisFlow *flow) :
		Flow(flow->basic) {
	this->basic = flow->basic;
	this->value = flow->value;
}

// TODO : Fix the join... See the commented out stuff
//Merges flow together.
Flow* RangeAnalysisFlow::join(Flow* otherSuper) {
	//join bottom-bottom gives you bottom. Anything else gives you top.
	RangeAnalysisFlow* other =
			static_cast<RangeAnalysisFlow*>(otherSuper);
//	errs()<< "I just entered into the sublcassed join... \n";

	if (this->basic == BOTTOM && other->basic == BOTTOM)
		return new RangeAnalysisFlow(BOTTOM);

	//Anything joined with a bottom will just be itself.
	if (this->basic == BOTTOM) {
		RangeAnalysisFlow* f = new RangeAnalysisFlow();
		f->copy(other);
		return f;
	}
	if (other->basic == BOTTOM) {
		RangeAnalysisFlow* f = new RangeAnalysisFlow();
		f->copy(this);
		return f;
	}


	//Join anything with top will give you top.
	if (this->basic == TOP || other->basic == TOP)
		return new RangeAnalysisFlow(TOP);

//Merge the input from both.
	RangeAnalysisFlow* f = new RangeAnalysisFlow(other);
	for (map<string, RangeDomainElement>::iterator it = this->value.begin(); it != this->value.end(); it++) {
				if (f->value.find(it->first) == f->value.end()) {
					errs() << "New key\n";
					// They don't have the same key! We're good!
					f->value[it->first] = it->second;
				} else {
					errs() << "Existing key\n";
					// Oh no! They do have the same key! We need to check if they have
					// the same values! if they do then we're good
					RangeDomainElement otherVal = other->value.find(it->first)->second;
					RangeDomainElement thisVal = this->value.find(it->first)->second;
					RangeDomainElement joinedVal = JoinRangeDomainElements(	(const RangeDomainElement*) &otherVal,
							(const RangeDomainElement*) &thisVal);
					f->value[it->first] = joinedVal;
				}
	}



////DEBUG TO CHECK JOIN WORKS
//int mycount, hiscount, lastcount;
//mycount = this->value.size();
//hiscount = other->value.size();
////END DEBUG TO CHECK JOIN WORKS
//
//
//	//f = other;
//	for (map<string, RangeDomainElement>::iterator it = this->value.begin();
//			it != this->value.end(); it++) {
//
//		if (other->value.find(it->first) == other->value.end()) {
//			// They don't have the same key! We're good!
//			f->value[it->first] = this->value.find(it->first)->second;
//		} else {
//			// Oh no! They do have the same key! We need to check if they have
//			// the same values! if they do then we're good
//			RangeDomainElement otherVal = other->value.find(it->first)->second;
//			RangeDomainElement thisVal = this->value.find(it->first)->second;
//			RangeDomainElement joinedVal = JoinRangeDomainElements(	(const RangeDomainElement*) &otherVal,
//					(const RangeDomainElement*) &thisVal);
//			f->value[it->first] = joinedVal;
//		}
//	}
//lastcount = other->value.size();

	return f;

}

RangeAnalysisFlow::~RangeAnalysisFlow() {
	//Nothing for basic static analysis
}

//UTILITY FUNCTIONS
//TO DO: DEBUG THIS CODE
RangeDomainElement JoinRangeDomainElements(const RangeDomainElement* A, const RangeDomainElement* B)
{
	//This will be the largest range possible. When you join ranges, you take the least precise range possible
	//WARNING!!!! Requires that you dont have values out of range!!!!!
	//WARNING!!!!

	RangeDomainElement maxAB;

	//Sanity checks. Return bottom if both A and B have not had bottom cleared
	if((A->bottom == true) && (B->bottom == true))
		return maxAB;	//Just return the default domain element, if no one was smart enough to clear bottom.
	//Return B if A is bottom, since A has no range
	if(A->bottom == true)
		{return *B;}
	//Return A if B is bottom, since B has no range
	if(B->bottom == true)
		{return *A;}


	//If A has the lowest of the low range, make maxAB take that value.
	maxAB.lower = (A->lower <= B->lower) ? A->lower : B->lower;
	//If A has the highest of the high range, make maxAB take that value.
	maxAB.upper = (A->upper >= B->upper) ? A->upper : B->upper;
	//Preserve the TOP if it has been set in one of these variables
	maxAB.top = (A->top | B->top);
	//clear the BOTTOM if it has been cleared in both of these variables
	maxAB.bottom = (A->bottom & B->bottom);

	return maxAB;
}
//TO DO: DEBUG THIS CODE
bool RangeDomainElementisEqual(const RangeDomainElement* A, const RangeDomainElement* B)
{
	if(A->lower == B->lower)
	{
		if(A->upper == B->upper)
		{
			return true;
		}
	}
	return false;
}

//END UTILITY FUNCTIONS
