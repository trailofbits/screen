/**
 * \file AbstractMan.h
 * \brief Declaration of the AbstractMan* classes
 * \author Julien Henry
 */
#ifndef _ABSTRACTMAN_H
#define _ABSTRACTMAN_H

#include "ap_global1.h"
#include "Abstract.h"
#include "Environment.h"

/**
 * \class AbstractMan
 * \brief class for creating Abstract objects
 */
class AbstractMan {

	public:
		/**
		 * \brief creates an object of type Abstract
		 * \param man apron manager 
		 * \param env environment of the created abstract value
		 */
		virtual Abstract * NewAbstract(ap_manager_t * man, Environment * env) = 0;

		/**
		 * \brief copy an Abstract object
		 * \param A the abstract value
		 */
		virtual Abstract * NewAbstract(Abstract * A) = 0;

		virtual ~AbstractMan() {};
};

/**
 * \class AbstractManClassic
 * \brief class that create Abstract objects of type AbstractClassic
 */
class AbstractManClassic : public AbstractMan {

	public:
		/**
		 * \brief creates an object of type AbstractClassic
		 * \param man apron manager 
		 * \param env environment of the created abstract value
		 */
		Abstract * NewAbstract(ap_manager_t * man, Environment * env);
		/**
		 * \brief copy an Abstract object
		 * \param A the abstract value
		 */
		Abstract * NewAbstract(Abstract * A);
};

/**
 * \class AbstractManGopan
 * \brief class that create Abstract objects of type AbstractGopan
 */
class AbstractManGopan : public AbstractMan {

	public:
		/**
		 * \brief creates an object of type AbstractGopan
		 * \param man apron manager 
		 * \param env environment of the created abstract value
		 */
		Abstract * NewAbstract(ap_manager_t * man, Environment * env);
		/**
		 * \brief copy an Abstract object
		 * \param A the abstract value
		 */
		Abstract * NewAbstract(Abstract * A);
};

/**
 * \class AbstractManDisj
 * \brief class that create Abstract objects of type AbstractDisj
 */
class AbstractManDisj : public AbstractMan {

	public:
		/**
		 * \brief creates an object of type AbstractDisj
		 * \param man apron manager 
		 * \param env environment of the created abstract value
		 */
		Abstract * NewAbstract(ap_manager_t * man, Environment * env);
		/**
		 * \brief copy an Abstract object
		 * \param A the abstract value
		 */
		Abstract * NewAbstract(Abstract * A);
};
#endif
