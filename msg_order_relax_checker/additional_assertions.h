/*
 * additional_assertions.h
 *
 *  Created on: 03.04.2020
 *      Author: Tim Jammer
 */

#ifndef MSG_ORDER_RELAX_CHECKER_ADDITIONAL_ASSERTIONS_H_
#define MSG_ORDER_RELAX_CHECKER_ADDITIONAL_ASSERTIONS_H_

#include "llvm/IR/Module.h"

bool check_no_any_tag(llvm::Module &M);
bool check_no_any_source(llvm::Module &M);

bool check_exact_length(llvm::Module &M);

#endif /* MSG_ORDER_RELAX_CHECKER_ADDITIONAL_ASSERTIONS_H_ */
