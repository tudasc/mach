/*
 * conflict_detection.h
 *
 *  Created on: 25.03.2020
 *      Author: Tim Jammer
 */

#ifndef MSG_ORDER_RELAX_CHECKER_CONFLICT_DETECTION_H_
#define MSG_ORDER_RELAX_CHECKER_CONFLICT_DETECTION_H_

#include "llvm/IR/InstrTypes.h"

#include <vector>

std::vector<std::pair<llvm::CallBase *, llvm::CallBase *>>
check_mpi_recv_conflicts(llvm::Module &M);

std::vector<std::pair<llvm::CallBase *, llvm::CallBase *>>
check_mpi_send_conflicts(llvm::Module &M);

llvm::Value *get_communicator(llvm::CallBase *mpi_call);
llvm::Value *get_src(llvm::CallBase *mpi_call, bool is_send);
llvm::Value *get_tag(llvm::CallBase *mpi_call, bool is_send);

#endif /* MSG_ORDER_RELAX_CHECKER_CONFLICT_DETECTION_H_ */
