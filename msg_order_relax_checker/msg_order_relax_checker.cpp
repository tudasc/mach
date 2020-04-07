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
#include "llvm/Analysis/TargetLibraryInfo.h"

#include <assert.h>
//#include <mpi.h>
#include <cstring>
#include <utility>
#include <vector>

#include "additional_assertions.h"
#include "conflict_detection.h"
#include "function_coverage.h"
#include "implementation_specific.h"
#include "mpi_functions.h"

using namespace llvm;

std::map<llvm::Function *, llvm::AliasAnalysis *> AA;
// declare dso_local i32 @MPI_Recv(i8*, i32, i32, i32, i32, i32,
// %struct.MPI_Status*) #1

struct mpi_functions *mpi_func;
struct implementation_specific_constants *mpi_implementation_specific_constants;
FunctionMetadata *function_metadata;

namespace {
struct MSGOrderRelaxCheckerPass : public ModulePass {
  static char ID;

  MSGOrderRelaxCheckerPass() : ModulePass(ID) {}

  // register that we require this analysis
  void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<TargetLibraryInfoWrapperPass>();
    AU.addRequiredTransitive<AAResultsWrapperPass>();
  }

  // Pass starts here
  virtual bool runOnModule(Module &M) {

    // debug:
    M.dump();

    mpi_func = get_used_mpi_functions(M);
    if (!is_mpi_used(mpi_func)) {
      // nothing to do for non mpi applicatiopns
      delete mpi_func;
      return false;
    }

    // give it any function, the Function is not used at all
    // dont know why the api has changed here...
    TargetLibraryInfo *TLI =
        &getAnalysis<TargetLibraryInfoWrapperPass>().getTLI(
            *mpi_func->mpi_init);

    for (auto &func : M) {
      if (!func.isDeclaration()) {
        auto *result = &getAnalysis<AAResultsWrapperPass>(func).getAAResults();
        AA.insert(std::make_pair(&func, result));
      }
    }

    function_metadata = new FunctionMetadata(TLI, M);

    mpi_implementation_specific_constants = get_implementation_specifics(M);

    std::vector<std::pair<llvm::CallBase *, llvm::CallBase *>> send_conflicts =
        check_mpi_send_conflicts(M);

    std::vector<std::pair<llvm::CallBase *, llvm::CallBase *>> recv_conflicts =
        check_mpi_recv_conflicts(M);

    if (!send_conflicts.empty() || !recv_conflicts.empty())
      errs() << "Message race conflicts detected\n";
    else {
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

    errs() << "Successfully executed the example pass\n\n";
    delete mpi_func;
    delete mpi_implementation_specific_constants;

    delete function_metadata;

    return false;
  }
};
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
