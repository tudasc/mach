/*
 * implementation_specific.cpp
 *
 *  Created on: 25.03.2020
 *      Author: Tim Jammer
 */

#include "implementation_specific.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Module.h"

#include <mpi.h>

using namespace llvm;

ImplementationSpecifics::ImplementationSpecifics(Module &M) {

  // need it to use MPI_Type_size
  MPI_Init(NULL, NULL);

  COMM_WORLD =
      ConstantInt::get(IntegerType::get(M.getContext(), 32), MPI_COMM_WORLD);
  ANY_TAG = ConstantInt::get(IntegerType::get(M.getContext(), 32), MPI_ANY_TAG);
  ANY_SOURCE =
      ConstantInt::get(IntegerType::get(M.getContext(), 32), MPI_ANY_SOURCE);
}
ImplementationSpecifics::~ImplementationSpecifics() { MPI_Finalize(); }

int ImplementationSpecifics::get_size_of_mpi_type(llvm::Constant *type) {
  // TODO if DataType is no integer type, this will break...

  if (auto *i = dyn_cast<ConstantInt>(type)) {
    auto mpi_type = i->getSExtValue();

    int size = 0;
    MPI_Type_size(mpi_type, &size);
    return size;
  }

  else {
    assert(false);
  }
}
