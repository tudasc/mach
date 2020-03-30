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

#include "llvm/Analysis/TargetLibraryInfo.h"

#include <assert.h>
//#include <mpi.h>
#include <cstring>
#include <utility>
#include <vector>

#include "conflict_detection.h"
#include "implementation_specific.h"
#include "mpi_functions.h"

using namespace llvm;

// declare dso_local i32 @MPI_Recv(i8*, i32, i32, i32, i32, i32,
// %struct.MPI_Status*) #1

struct mpi_functions *mpi_func;
struct implementation_specific_constants *mpi_implementation_specific_constants;

std::set<Function *> get_stdlib_functions(Module &M,
                                          const TargetLibraryInfo *TLI) {
  LibFunc libF;

  std::set<Function *> result;

  for (auto &F : M) {

    if (TLI->getLibFunc(F, libF)) {
      errs() << F.getName() << " is PART OF stdlibs\n";
    } else {
      errs() << F.getName() << " is user defined (or another library )\n";
    }
  }

  return result;
}

namespace {
struct MSGOrderRelaxCheckerPass : public ModulePass {
  static char ID;

  MSGOrderRelaxCheckerPass() : ModulePass(ID) {}

  // register that we require this analysis
  void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<TargetLibraryInfoWrapperPass>();
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

    const TargetLibraryInfo *TLI =
        &getAnalysis<TargetLibraryInfoWrapperPass>().getTLI();

    get_stdlib_functions(M, TLI);

    mpi_implementation_specific_constants = get_implementation_specifics(M);

    bool conflicts = false;
    conflicts = conflicts || check_mpi_send_conflicts(M);
    conflicts = conflicts || check_mpi_Isend_conflicts(M);
    conflicts = conflicts || check_mpi_Bsend_conflicts(M);
    conflicts = conflicts || check_mpi_Ssend_conflicts(M);
    conflicts = conflicts || check_mpi_Rsend_conflicts(M);

    if (conflicts)
      errs() << "Message race conflicts detected\n";
    else {
      errs() << "No conflicts detected, try to use mpi_assert_allow_overtaking "
                "for better performance\n";
    }
    errs() << "Successfully executed the example pass\n\n";
    delete mpi_func;
    delete mpi_implementation_specific_constants;

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
