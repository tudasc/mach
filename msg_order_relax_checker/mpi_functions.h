/*
 * mpi_functions.h
 *
 *  Created on: 22.03.2020
 *      Author: Tim Jammer
 */

#ifndef MSG_ORDER_RELAX_CHECKER_MPI_FUNCTIONS_H_
#define MSG_ORDER_RELAX_CHECKER_MPI_FUNCTIONS_H_

#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"

struct mpi_functions {
  llvm::Function *mpi_init;
  llvm::Function *mpi_finalize;
  llvm::Function *mpi_send;
  llvm::Function *mpi_recv;
  llvm::Function *mpi_barrier;
};

struct mpi_functions *get_used_mpi_functions(llvm::Module &M);

bool is_mpi_used(struct mpi_functions *mpi_func);

#endif /* MSG_ORDER_RELAX_CHECKER_MPI_FUNCTIONS_H_ */
