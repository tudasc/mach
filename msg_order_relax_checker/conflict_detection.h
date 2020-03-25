/*
 * conflict_detection.h
 *
 *  Created on: 25.03.2020
 *      Author: Tim Jammer
 */

#ifndef MSG_ORDER_RELAX_CHECKER_CONFLICT_DETECTION_H_
#define MSG_ORDER_RELAX_CHECKER_CONFLICT_DETECTION_H_

#include "llvm/IR/InstrTypes.h"

bool are_calls_conflicting(llvm::CallBase *orig_call,
                           llvm::CallBase *conflict_call);

llvm::Value *get_communicator(llvm::CallBase *mpi_call);
llvm::Value *get_src(llvm::CallBase *mpi_call);
llvm::Value *get_tag(llvm::CallBase *mpi_call);

#endif /* MSG_ORDER_RELAX_CHECKER_CONFLICT_DETECTION_H_ */
