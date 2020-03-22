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
	llvm::Function *mpi_init = nullptr;
	llvm::Function *mpi_send = nullptr;
	llvm::Function *mpi_recv = nullptr;
	llvm::Function *mpi_finalize = nullptr;
};

struct mpi_functions * get_used_mpi_functions(llvm::Module &M);

#endif /* MSG_ORDER_RELAX_CHECKER_MPI_FUNCTIONS_H_ */
