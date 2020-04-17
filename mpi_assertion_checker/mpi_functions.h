/*
  Copyright 2020 Tim Jammer

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#ifndef MACH_MPI_FUNCTIONS_H_
#define MACH_MPI_FUNCTIONS_H_

#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
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
  llvm::Function *mpi_Bsend = nullptr;
  llvm::Function *mpi_Ssend = nullptr;
  llvm::Function *mpi_Rsend = nullptr;
  llvm::Function *mpi_Isend = nullptr;
  llvm::Function *mpi_Ibsend = nullptr;
  llvm::Function *mpi_Issend = nullptr;
  llvm::Function *mpi_Irsend = nullptr;

  llvm::Function *mpi_Sendrecv = nullptr;

  llvm::Function *mpi_recv = nullptr;
  llvm::Function *mpi_Irecv = nullptr;

  llvm::Function *mpi_test = nullptr;
  llvm::Function *mpi_wait = nullptr;
  llvm::Function *mpi_waitall = nullptr;
  llvm::Function *mpi_buffer_detach = nullptr;

  llvm::Function *mpi_barrier = nullptr;
  llvm::Function *mpi_allreduce = nullptr;
  llvm::Function *mpi_Ibarrier = nullptr;
  llvm::Function *mpi_Iallreduce = nullptr;

  std::set<llvm::Function *>
      conflicting_functions; // may result in a conflict for msg overtaking
  std::set<llvm::Function *>
      sync_functions; // will end the conflicting timeframe (like a barrier)
  std::set<llvm::Function *>
      unimportant_functions; // no implications for msg overtaking
};

struct mpi_functions *get_used_mpi_functions(llvm::Module &M);

bool is_mpi_used(struct mpi_functions *mpi_func);

bool is_mpi_call(llvm::CallBase *call);
bool is_mpi_function(llvm::Function *f);

bool is_send_function(llvm::Function *f);
bool is_recv_function(llvm::Function *f);

#endif /* MACH_MPI_FUNCTIONS_H_ */
