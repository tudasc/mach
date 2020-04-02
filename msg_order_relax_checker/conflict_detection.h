/*
 * conflict_detection.h
 *
 *  Created on: 25.03.2020
 *      Author: Tim Jammer
 */

#ifndef MSG_ORDER_RELAX_CHECKER_CONFLICT_DETECTION_H_
#define MSG_ORDER_RELAX_CHECKER_CONFLICT_DETECTION_H_

#include "llvm/IR/InstrTypes.h"

bool check_mpi_recv_conflicts(llvm::Module &M);
bool check_mpi_Irecv_conflicts(llvm::Module &M);

bool check_mpi_sendrecv_conflicts(llvm::Module &M);

bool check_mpi_send_conflicts(llvm::Module &M);
bool check_mpi_Isend_conflicts(llvm::Module &M);
bool check_mpi_Bsend_conflicts(llvm::Module &M);
bool check_mpi_Ssend_conflicts(llvm::Module &M);
bool check_mpi_Rsend_conflicts(llvm::Module &M);

bool check_conflicts(llvm::Module &M, llvm::Function *f, bool is_send);
bool check_call_for_conflict(llvm::CallBase *mpi_call, bool is_send);

bool are_calls_conflicting(llvm::CallBase *orig_call,
                           llvm::CallBase *conflict_call, bool is_send);

llvm::Value *get_communicator(llvm::CallBase *mpi_call);
llvm::Value *get_src(llvm::CallBase *mpi_call, bool is_send);
llvm::Value *get_tag(llvm::CallBase *mpi_call, bool is_send);

#endif /* MSG_ORDER_RELAX_CHECKER_CONFLICT_DETECTION_H_ */
