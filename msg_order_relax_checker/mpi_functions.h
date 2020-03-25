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

#include <set>

// global:
// will be init and destroyed in the Passes runOnModule function (equivalent to
// main)
extern struct mpi_functions *mpi_func;

struct mpi_functions {
  llvm::Function *mpi_init = nullptr;
  llvm::Function *mpi_finalize = nullptr;
  llvm::Function *mpi_send = nullptr;
  llvm::Function *mpi_recv = nullptr;
  llvm::Function *mpi_barrier = nullptr;
  std::set<llvm::Function *>
      conflicting_functions; // may result in a conflict for msg overtaking
  std::set<llvm::Function *>
      sync_functions; // will end the conflicting timeframe (like a barrier)
  std::set<llvm::Function *>
      unimportant_functions; // no implications for msg overtaking
};

struct mpi_functions *get_used_mpi_functions(llvm::Module &M);

bool is_mpi_used(struct mpi_functions *mpi_func);

#endif /* MSG_ORDER_RELAX_CHECKER_MPI_FUNCTIONS_H_ */
