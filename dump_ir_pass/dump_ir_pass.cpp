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

//#include <mpi.h>
#include <cstring>
#include <utility>
using namespace llvm;

namespace {
struct ExperimentPass : public ModulePass {
  static char ID;
  ExperimentPass() : ModulePass(ID) {}

  // Pass starts here
  virtual bool runOnModule(Module &M) {
    M.dump();
    printf("Succesfully executed the example pass\n\n");
    return false;
  }
};
} // namespace

char ExperimentPass::ID = 0;

// Automatically enable the pass.
// http://adriansampson.net/blog/clangpass.html
static void registerExperimentPass(const PassManagerBuilder &,
                                   legacy::PassManagerBase &PM) {
  PM.add(new ExperimentPass());
}

static RegisterStandardPasses
    RegisterMyPass(PassManagerBuilder::EP_ModuleOptimizerEarly,
                   registerExperimentPass);

static RegisterStandardPasses
    RegisterMyPass0(PassManagerBuilder::EP_EnabledOnOptLevel0,
                    registerExperimentPass);
