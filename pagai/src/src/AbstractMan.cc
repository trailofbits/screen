/**
 * \file AbstractMan.cc
 * \brief Implementation of the AbstractMan* classes
 * \author Julien Henry
 */
#include "Analyzer.h"
#include "AbstractMan.h"
#include "AbstractClassic.h"
#include "AbstractGopan.h"
#include "AbstractDisj.h"

Abstract * AbstractManClassic::NewAbstract(ap_manager_t * man, Environment * env) {
	return new AbstractClassic(man,env);
}

Abstract * AbstractManClassic::NewAbstract(Abstract * A) {
	return new AbstractClassic(A);
}

Abstract * AbstractManGopan::NewAbstract(ap_manager_t * man, Environment * env) {
	return new AbstractGopan(man,env);
}

Abstract * AbstractManGopan::NewAbstract(Abstract * A) {
	return new AbstractGopan(A);
}

Abstract * AbstractManDisj::NewAbstract(ap_manager_t * man, Environment * env) {
	return new AbstractDisj(man,env,0);
}

Abstract * AbstractManDisj::NewAbstract(Abstract * A) {
	return new AbstractDisj(A);
}
