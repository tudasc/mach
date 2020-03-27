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

bool is_mpi_call(CallBase *call) {
  return call->getCalledFunction()->getName().contains("MPI");
}

bool check_call_for_conflict(CallBase *mpi_call) {

  std::set<BasicBlock *> to_check;
  std::set<BasicBlock *>
      already_checked; // be aware, that revisiting the current block might be
                       // necessary if in a loop
  std::set<CallBase *> potential_conflicts;

  Instruction *current_inst = dyn_cast<Instruction>(mpi_call);
  assert(current_inst != nullptr);
  Instruction *next_inst = current_inst->getNextNode();

  // mpi_call->dump();

  while (next_inst != nullptr) {
    current_inst = next_inst;
    // current_inst->dump();

    if (auto *call = dyn_cast<CallBase>(current_inst)) {
      if (is_mpi_call(call)) {
        // check if this call is conflicting or if search can be stopped as this
        // is a sync point
        // errs() << "need to check call to "
        //		<< call->getCalledFunction()->getName() << "\n";
        if (mpi_func->sync_functions.find(call->getCalledFunction()) !=
            mpi_func->sync_functions.end()) {
          // no need to analyze this path further, a sync point will stop msg
          // overtaking anyway
          current_inst = nullptr;
          errs() << "call to " << call->getCalledFunction()->getName()
                 << " is a sync point, no overtaking possible beyond it\n";
        } else if (mpi_func->conflicting_functions.find(
                       call->getCalledFunction()) !=
                   mpi_func->conflicting_functions.end()) {
          potential_conflicts.insert(call);
        } else if (mpi_func->unimportant_functions.find(
                       call->getCalledFunction()) !=
                   mpi_func->unimportant_functions.end()) {
          errs() << "call to " << call->getCalledFunction()->getName()
                 << " is currently not supported in this analysis\n";
        }

      } else {
        errs() << "Call To " << call->getCalledFunction()->getName()
               << "Currently no checking whether there is MPI there!\n"
               << "ASSUMING NO MPI IN " << call->getCalledFunction()->getName()
               << "\n";
      }
    }

    // now fetch the next inst
    next_inst = nullptr;

    if (current_inst !=
        nullptr) // if not discovered to stop analyzing this code path
    {
      if (current_inst->isTerminator()) {
        for (unsigned int i = 0; i < current_inst->getNumSuccessors(); ++i) {
          auto *next_block = current_inst->getSuccessor(i);
          if (already_checked.find(next_block) == already_checked.end()) {
            to_check.insert(next_block);
          }
        }
      }

      next_inst = current_inst->getNextNode();
    }

    if (next_inst == nullptr) {
      // errs() << to_check.size();
      if (!to_check.empty()) {
        auto it_pos = to_check.begin();

        BasicBlock *bb = *it_pos;
        to_check.erase(it_pos);
        already_checked.insert(
            bb); // will be checked now, so no revisiting necessary
        next_inst = bb->getFirstNonPHI();
      }
    }
  }

  // check for conflicts:
  for (auto *call : potential_conflicts) {
    bool conflict = are_calls_conflicting(mpi_call, call);
    if (conflict) {
      // found at least one conflict, currently we can stop then
      return true;
    }
  }

  // no conflict found
  return false;
}

bool check_mpi_send_conflicts(Module &M) {
  if (mpi_func->mpi_send == nullptr) {
    // no msg: no conflict
    return false;
  }

  for (auto user : mpi_func->mpi_send->users()) {
    if (CallBase *call = dyn_cast<CallBase>(user)) {
      if (call->getCalledFunction() == mpi_func->mpi_send) {
        if (check_call_for_conflict(call)) {
          // found conflict
          return true;
        }

      } else {
        errs() << "? " << call << "\nWhy do you do that?\n";
      }
    }
  }

  // no conflicts found
  return false;
}

struct mpi_functions *mpi_func;
struct implementation_specific_constants *mpi_implementation_specific_constants;

namespace {
struct MSGOrderRelaxCheckerPass : public ModulePass {
  static char ID;
  MSGOrderRelaxCheckerPass() : ModulePass(ID) {}

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

    mpi_implementation_specific_constants = get_implementation_specifics(M);

    bool conflicts = false;
    conflicts = conflicts || check_mpi_send_conflicts(M);

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
