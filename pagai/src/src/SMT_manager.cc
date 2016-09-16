/**
 * \file SMT_manager.cc
 * \brief Implementation of the SMT_manager class
 * \author Julien Henry
 */
#include "SMT_manager.h"

SMT_expr SMT_manager::SMT_mk_divides(SMT_expr a1, SMT_expr a2) {
  return SMT_mk_eq(SMT_mk_rem(a2, a1), SMT_mk_num(0));
}

bool SMT_manager::interrupt() {
  return false;
}
