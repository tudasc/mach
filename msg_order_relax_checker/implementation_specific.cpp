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

struct implementation_specific_constants *
get_implementation_specifics(Module &M) {

  struct implementation_specific_constants *result =
      new struct implementation_specific_constants;
  assert(result != nullptr);

  result->COM_WORLD =
      ConstantInt::get(IntegerType::get(M.getContext(), 32), MPI_COMM_WORLD);
  result->ANY_TAG =
      ConstantInt::get(IntegerType::get(M.getContext(), 32), MPI_ANY_TAG);
  result->ANY_SOURCE =
      ConstantInt::get(IntegerType::get(M.getContext(), 32), MPI_ANY_SOURCE);

  return result;
}
