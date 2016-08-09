/*

 * Flow.h
 *
 *  Created on: 2014-05-29
 *      Author: jtestard
 */

#ifndef CONSTANT_PROP_ANALYSIS_FLOW_H_
#define CONSTANT_PROP_ANALYSIS_FLOW_H_
#include <string>
#include <map>
#include <set>
#include <algorithm>
#include <sstream>
#include <limits>		//Has max int and min int.
#include "llvm/Support/raw_ostream.h"
#include "Flow.h"

using namespace std;
using namespace llvm;
/*
 * This class is the FLow class for the constant propagation analysis.
 * This is a May-Point-To Analysis.
 */

//this type represents the range for each variable.
typedef struct _RangeDomainElement{
	float upper;
	float lower;
	bool top;
	bool bottom;
	bool undefined;
	_RangeDomainElement()
	{
			upper = 0;//BOTTOM IS 0 RANGEstd::numeric_limits<float>::infinity();
			lower = 0;//BOTTOM IS 0 RANGE-std::numeric_limits<float>::infinity();
			top = false;
			bottom = true;
			undefined = false;
	}
}RangeDomainElement;

//Utility functions
RangeDomainElement JoinRangeDomainElements(const RangeDomainElement* A, const RangeDomainElement* B);
bool RangeDomainElementisEqual(const RangeDomainElement* A, const RangeDomainElement* B);
//End utility functions

class RangeAnalysisFlow: public Flow {

public:

	//The equality operator is used by the worklist algorithm and must be overloaded by the analysis.
	bool equals(Flow* other);

	/* This method is used by the JSONCFG function of the analysis to output the graph in JSON format.
	 * It must output a proper representation of the flow in JSON format :
	 *
	 * 		{ "X" : "Z", "Z" : ["W","Y"] }
	 *
	 * 	Where the left hand side are variable names and right hand side are also variable names.
	 */
	string jsonString();

	/*
	 * The equality operator must also be overloaded when we want to assign a variable to top or bottom (or something else).
	 */
	void copy(Flow* rhs);

	/*
	 * The join function must be overloaded by the Flow subclasses.
	 */
	Flow* join(Flow* other);

	//This constructor initializes an empty map.
	RangeAnalysisFlow();

	//This constructor should be used for top or bottom.
	RangeAnalysisFlow(string input);

	//Required for type casting within overloaded functions.
	RangeAnalysisFlow(RangeAnalysisFlow* flow);

	~RangeAnalysisFlow();

	//Variables are represented as strings.
	map<string, RangeDomainElement > value;

};

#endif /* CONSTANT_PROP_ANALYSIS_FLOW_H_ */
