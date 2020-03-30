/*
 * function_coverage.h
 *
 *  Created on: 30.03.2020
 *      Author: Tim Jammer
 */

#ifndef MSG_ORDER_RELAX_CHECKER_FUNCTION_COVERAGE_H_
#define MSG_ORDER_RELAX_CHECKER_FUNCTION_COVERAGE_H_

#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/IR/Function.h"

#include <map>
#include <tuple>

// this class does the per function analysis
// it stores if a function uses MPI that may conflict
class FunctionMetadata {
public:
  FunctionMetadata(const llvm::TargetLibraryInfo *TLI, llvm::Module &M);
  ~FunctionMetadata(){};

  bool has_mpi(llvm::Function *F);
  bool may_conflict(llvm::Function *F);
  bool will_sync(llvm::Function *F);

  bool is_unknown(llvm::Function *F);

private:
  // maps function to tuple of bool:
  // unknown, has mpi, has sync, may conflict
  // unknown means definition not within this module and not part of stdlib (and
  // no mpi call itself)

  std::map<llvm::Function *, std::tuple<bool, bool, bool, bool>>
      function_metadata;
};

// global will be managed by main
extern FunctionMetadata *function_metadata;

#endif /* MSG_ORDER_RELAX_CHECKER_FUNCTION_COVERAGE_H_ */
