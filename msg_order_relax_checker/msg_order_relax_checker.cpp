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

#include "mpi_functions.h"

using namespace llvm;

// declare dso_local i32 @MPI_Recv(i8*, i32, i32, i32, i32, i32,
// %struct.MPI_Status*) #1

struct mpi_functions *mpi_func = nullptr;

bool is_mpi_call(CallBase *call) {
  return call->getCalledFunction()->getName().contains("MPI");
}

bool are_calls_conflicting(CallBase *orig_call, CallBase *conflict_call) {

  errs() << "\n";
  orig_call->dump();
  errs() << "potential conflict detected: ";
  conflict_call->dump();
  errs() << "\n";
  if (orig_call == conflict_call) {
    errs() << "Recv is conflicting with itself, probably in a loop, if using "
              "different msg tags on each iteration this is safe nontheless\n";
    return true;
  }

  assert((orig_call->getCalledFunction() == mpi_func->mpi_recv &&
          conflict_call->getCalledFunction() == mpi_func->mpi_recv) &&
         "Currently only MPI_Recv is supported");

  assert(orig_call->getNumArgOperands() == 7);
  assert(conflict_call->getNumArgOperands() == 7);
  unsigned int communicator_arg_pos = 5;
  unsigned int src_arg_pos = 3;
  unsigned int tag_arg_pos = 4;

  // check communicator
  auto *comm1 = orig_call->getArgOperand(communicator_arg_pos);
  auto *comm2 = conflict_call->getArgOperand(communicator_arg_pos);

  if (auto *c1 = dyn_cast<Constant>(comm1)) {
    if (auto *c2 = dyn_cast<Constant>(comm2)) {
      if (c1 != c2) {
        // different communicators
        return false;
      }
    }
  } // otherwise, we have not proven that the communicator is be different
    // TODO: (very advanced) if e.g. mpi comm split is used, we might be able to
    // statically prove different communicators

  // check src
  auto *src1 = orig_call->getArgOperand(src_arg_pos);
  auto *src2 = conflict_call->getArgOperand(src_arg_pos);
  if (auto *s1 = dyn_cast<Constant>(src1)) {
    if (auto *s2 = dyn_cast<Constant>(src2)) {
      if (s1 != s2) {
        // different sources
        // TODO we need to exclude any MPI_ANY_SOURCE here
        errs() << "If you use MPI_ANY_SOURCE in any MPI call, this analysis "
                  "might be screwed, as this is currently not supported";
        return false;
      }
    }
  } // otherwise, we have not proven that the src might be different
  src1->dump();

  // check tag
  auto *tag1 = orig_call->getArgOperand(tag_arg_pos);
  auto *tag2 = conflict_call->getArgOperand(tag_arg_pos);
  if (auto *t1 = dyn_cast<Constant>(tag1)) {
    if (auto *t2 = dyn_cast<Constant>(tag2)) {
      if (t1 != t2) {
        // different tags
        // TODO we need to exclude any MPI_ANY_TAG here
        errs() << "If you use MPI_ANY_TAG in any MPI call, this analysis might "
                  "be screwed, as this is currently not supported";
        return false;
      }
    }
  } // otherwise, we have not proven that the tag is be different
  tag1->dump();

  // cannot disprove conflict, have to assume it indeed relays on msg ordering
  return true;
}

bool check_call_for_conflict(CallBase *mpi_call) {

  std::set<BasicBlock *> to_check;
  std::set<BasicBlock *>
      already_checked; // be aware, that revisiting the current block micht be
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

bool check_mpi_recv_conflicts(Module &M) {
  if (mpi_func->mpi_recv == nullptr) {
    // no msg: no conflict
    return false;
  }

  for (auto user : mpi_func->mpi_recv->users()) {
    if (CallBase *call = dyn_cast<CallBase>(user)) {
      if (call->getCalledFunction() == mpi_func->mpi_recv) {
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

    bool conflicts = false;
    conflicts = conflicts || check_mpi_recv_conflicts(M);

    if (conflicts)
      errs() << "Message race conflicts detected\n";
    else {
      errs() << "No conflicts detected, try to use mpi_assert_allow_overtaking "
                "for better performance\n";
    }
    errs() << "Successfully executed the example pass\n\n";
    delete mpi_func;

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
