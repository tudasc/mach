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

#ifndef MACH_FUNCTION_COVERAGE_H_
#define MACH_FUNCTION_COVERAGE_H_

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
