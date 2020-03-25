/*
 * implementation_specific.h
 *
 *  Created on: 25.03.2020
 *      Author: Tim Jammer
 */

#ifndef MSG_ORDER_RELAX_CHECKER_IMPLEMENTATION_SPECIFIC_H_
#define MSG_ORDER_RELAX_CHECKER_IMPLEMENTATION_SPECIFIC_H_

#include "llvm/IR/Constant.h"
struct implementation_specific_constants {
  llvm::Constant *COM_WORLD;
  llvm::Constant *ANY_SOURCE;
  llvm::Constant *ANY_TAG;
};

// created and deleted in main
extern struct implementation_specific_constants
    *mpi_implementation_specific_constants;

struct implementation_specific_constants *
get_implementation_specifics(llvm::Module &M);

#endif /* MSG_ORDER_RELAX_CHECKER_IMPLEMENTATION_SPECIFIC_H_ */
