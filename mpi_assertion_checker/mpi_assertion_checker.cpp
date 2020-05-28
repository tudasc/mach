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

#include "llvm/ADT/APInt.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/TargetLibraryInfo.h"

#include <assert.h>
//#include <mpi.h>
#include <cstring>
#include <utility>
#include <vector>

#include "additional_assertions.h"
#include "analysis_results.h"
#include "conflict_detection.h"
#include "debug.h"
#include "function_coverage.h"
#include "implementation_specific.h"
#include "mpi_functions.h"

using namespace llvm;

// declare dso_local i32 @MPI_Recv(i8*, i32, i32, i32, i32, i32,
// %struct.MPI_Status*) #1

RequiredAnalysisResults *analysis_results;

struct mpi_functions *mpi_func;
struct ImplementationSpecifics *mpi_implementation_specifics;
FunctionMetadata *function_metadata;

namespace {
struct MSGOrderRelaxCheckerPass : public ModulePass {
  static char ID;

  MSGOrderRelaxCheckerPass() : ModulePass(ID) {}

  // register that we require this analysis

  void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<TargetLibraryInfoWrapperPass>();
    AU.addRequiredTransitive<AAResultsWrapperPass>();
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addRequired<ScalarEvolutionWrapperPass>();
  }
  /*
   void getAnalysisUsage(AnalysisUsage &AU) const {
   AU.addRequiredTransitive<TargetLibraryInfoWrapperPass>();
   AU.addRequiredTransitive<AAResultsWrapperPass>();
   AU.addRequiredTransitive<LoopInfoWrapperPass>();
   AU.addRequiredTransitive<ScalarEvolutionWrapperPass>();
   }
   */

  StringRef getPassName() const { return "MPI Assertion Analysis"; }

  // Pass starts here
  virtual bool runOnModule(Module &M) {

    Debug(M.dump(););

    mpi_func = get_used_mpi_functions(M);
    if (!is_mpi_used(mpi_func)) {
      // nothing to do for non mpi applicatiopns
      delete mpi_func;
      return false;
    }

    analysis_results = new RequiredAnalysisResults(this);

    function_metadata = new FunctionMetadata(analysis_results->getTLI(), M);

    mpi_implementation_specifics = new ImplementationSpecifics(M);

    std::vector<std::pair<llvm::CallBase *, llvm::CallBase *>> send_conflicts =
        check_mpi_send_conflicts(M);

    std::vector<std::pair<llvm::CallBase *, llvm::CallBase *>> recv_conflicts =
        check_mpi_recv_conflicts(M);

    if (!send_conflicts.empty() || !recv_conflicts.empty()) {
      /*
          if (!send_conflicts.empty()) {
                  errs() << "send conflicts\n";
                  for (auto conflict : send_conflicts) {
                                                          conflict.first->dump();
                                                          conflict.second->dump();
                                                          errs()
<< "\n";
                                                  }
          }
          if (!recv_conflicts.empty()) {
                  errs() << "recv conflicts\n";

                  for (auto conflict : recv_conflicts) {
                          conflict.first->dump();
                          conflict.second->dump();
                          errs() << "\n";
                  }
          }
          */

      errs() << "Message race conflicts detected\n";
    } else {
      errs() << "No conflicts detected, try to use mpi_assert_allow_overtaking "
                "for better performance\n";
    }

    if (check_no_any_tag(M)) {
      errs() << "You can also safely specify mpi_assert_no_any_tag for better "
                "performance\n";
    }

    if (check_no_any_source(M)) {
      errs() << "You can also safely specify mpi_assert_no_any_source for "
                "better performance\n";
    }

    if (check_exact_length(M)) {
      errs() << "You can also safely specify mpi_assert_exact_length for "
                "better performance\n";
    }

    errs() << "Successfully executed the pass\n\n";
    delete mpi_func;
    delete mpi_implementation_specifics;
    delete analysis_results;

    delete function_metadata;

    return false;
  }
}; // class MSGOrderRelaxCheckerPass
} // namespace

char MSGOrderRelaxCheckerPass::ID = 42;

// Automatically enable the pass.
// http://adriansampson.net/blog/clangpass.html
static void registerExperimentPass(const PassManagerBuilder &,
                                   legacy::PassManagerBase &PM) {
  PM.add(new MSGOrderRelaxCheckerPass());
}

// static RegisterStandardPasses
//    RegisterMyPass(PassManagerBuilder::EP_ModuleOptimizerEarly,
//                   registerExperimentPass);

static RegisterStandardPasses
    RegisterMyPass(PassManagerBuilder::EP_OptimizerLast,
                   registerExperimentPass);

static RegisterStandardPasses
    RegisterMyPass0(PassManagerBuilder::EP_EnabledOnOptLevel0,
                    registerExperimentPass);
