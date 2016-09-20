/**
 * \file Info.h
 * \brief Declaration of the Info class
 * \author Rahul Nanda, Julien Henry
 */
#ifndef _INFO_H
#define _INFO_H

#include<string>
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Analysis/CFG.h"

extern llvm::raw_ostream *Out;

/**
 * \class Info
 * \brief Info class holds information about a variable in original source-code and provides necessary functions.
 */
class Info
{
	std::string name;
	int lineNo;
	std::string type;
	
	bool islocal; 
	bool isarg; 
	bool isret; 
	bool isglobal;

	public:
	
	
	Info() {
		lineNo=-1;
	}

	Info(std::string _name,int _lineNo,std::string _type, 
			bool _islocal = true, bool _isarg = false, bool _isret = false, bool _isglobal = false) {
		name=_name;
		lineNo=_lineNo;
		type=_type;
		islocal = _islocal; 
		isarg = _isarg; 
		isret = _isret; 
		isglobal = _isglobal;
	}

	Info(const Info& I): name(I.name), lineNo(I.lineNo), type(I.type),
	islocal(I.islocal), isarg(I.isarg), isret(I.isret), isglobal(I.isglobal) {}

	Info& operator=(const Info &I) {
		name = I.name;
		lineNo = I.lineNo;
		type = I.type;
		islocal = I.islocal; 
		isarg = I.isarg; 
		isret = I.isret; 
		isglobal = I.isglobal;
		return *this;
	}

	void display() const {
		*Out<<"(type="<<type<<" name="<<name<<" line="<<lineNo << " [";
		if (islocal) *Out << "local";
		if (isarg) *Out << "arg";
		if (isret) *Out << "ret";
		if (isglobal) *Out << "global";
		*Out<<"])";

	}

	std::string getName() const {return name;}

	std::string getType() {return type;}

	int getLineNo() {return lineNo;}

	bool empty() {return name.empty();}

	bool IsLocal() {return islocal;} 
	bool IsArg() {return isarg;} 
	bool IsRet() {return isret;}
	bool IsGlobal() {return isglobal;}

	int Compare (const Info& i) const {
		int name_comparison = name.compare(i.name);
		int type_comparison = type.compare(i.type);
		if(type_comparison == 0 && name_comparison == 0 && lineNo==i.lineNo) return 0;
		if(lineNo < i.lineNo) {
			return -1;
		} else if (lineNo > i.lineNo) {
			return 1;
		} else {
			if (name_comparison != 0) return name_comparison;
			return type_comparison;
		}
	}

	bool operator == (const Info& i) const {
		return !Compare(i);
	}

	bool operator < (const Info& i) const {
		return Compare(i)<0;   
	}
};

#endif
