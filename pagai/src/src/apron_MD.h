/**
 * \file apron_MD.h
 * \brief Declares functions related to the Apron interface
 * \author Julien Henry
 */
#ifndef _APRON_MD_H 
#define _APRON_MD_H 

#include "llvm/Analysis/CFG.h"
#include "llvm/IR/IRBuilder.h"

#include "ap_global1.h"

#include "Analyzer.h"
#include "Node.h"

using namespace llvm;

void ap_tcons1_t_to_MDNode(ap_tcons1_t & cons, llvm::Instruction * Inst, std::vector<llvm::Metadata*> * met);

void coeff_to_MDNode(ap_coeff_t * a, llvm::Instruction * Inst, std::vector<llvm::Metadata*> * met);
void ap_texpr1_t_to_MDNode(ap_texpr1_t & expr, llvm::Instruction * Inst, std::vector<llvm::Metadata*> * met);

void texpr0_to_MDNode(ap_texpr0_t* a, ap_environment_t * env, llvm::Instruction * Inst, std::vector<llvm::Metadata*> * met);
void texpr0_node_to_MDNode(ap_texpr0_node_t * a, ap_environment_t * env, llvm::Instruction * Inst, std::vector<llvm::Metadata*> * met);


Value * ap_tcons1_to_LLVM(ap_tcons1_t & cons, IRBuilder<> * Builder);
Value * ap_texpr1_to_LLVM(ap_texpr1_t & expr, IRBuilder<> * Builder); 
Value * texpr0_to_LLVM(ap_texpr0_t* a, ap_environment_t * env, IRBuilder<> * Builder);
Value * texpr0_node_to_LLVM(ap_texpr0_node_t * a, ap_environment_t * env, IRBuilder<> * Builder);
Value * ap_scalar_to_LLVM(ap_scalar_t & scalar, IRBuilder<> * Builder);
Value * coeff_to_LLVM(ap_coeff_t * a, IRBuilder<> * Builder);
#endif

