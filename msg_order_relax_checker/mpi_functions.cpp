/*
 * mpi_functions.cpp
 *
 *  Created on: 22.03.2020
 *      Author: Tim Jammer
 */
#include "mpi_functions.h"
#include <assert.h>

#include <llvm/Support/raw_ostream.h>

using namespace llvm;

struct mpi_functions *get_used_mpi_functions(llvm::Module &M) {

  struct mpi_functions *result = new struct mpi_functions;
  assert(result != nullptr);

  for (auto it = M.begin(); it != M.end(); ++it) {
    Function *f = &*it;
    errs() << f->getName() << "\n";
    if (f->getName().contains("MPI_Init")) {
      result->mpi_init = f;
      // should not be called twice anyway, so no need to handle it
      // result->conflicting_functions.insert(f);
    } else if (f->getName().contains("MPI_Finalize")) {
      result->mpi_finalize = f;
      result->sync_functions.insert(f);
    } else if (f->getName().contains("MPI_Send")) {
      result->mpi_send = f;
      result->conflicting_functions.insert(f);
    } else if (f->getName().contains("MPI_Recv")) {
      result->mpi_recv = f;
      // no conflict with sending out msg that may overtake each other
      // maybe this blocking recv may even prevent a conflict
      result->unimportant_functions.insert(f);
    } else if (f->getName().contains("MPI_Barrier")) {
      result->mpi_barrier = f;
      result->sync_functions.insert(f);
    }
  }

  return result;
}

bool is_mpi_used(struct mpi_functions *mpi_func) {

  if (mpi_func->mpi_init != nullptr) {
    return mpi_func->mpi_init->getNumUses() > 0;
  } else {
    return false;
  }
}
