/*
 * conflict_detection.cpp
 *
 *  Created on: 25.03.2020
 *      Author: Tim Jammer
 */

#include "conflict_detection.h"
#include "implementation_specific.h"
#include "mpi_functions.h"

using namespace llvm;

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

bool check_conflicts(llvm::Module &M, llvm::Function *f) {
  if (f == nullptr) {
    // no messages: no conflict
    return false;
  }

  for (auto user : f->users()) {
    if (CallBase *call = dyn_cast<CallBase>(user)) {
      if (call->getCalledFunction() == f) {
        if (check_call_for_conflict(call)) {
          // found conflict
          return true;
        }

      } else {
        call->dump();
        errs() << "\nWhy do you do that?\n";
      }
    }
  }

  // no conflicts found
  return false;
}

bool check_mpi_send_conflicts(Module &M) {
  return check_conflicts(M, mpi_func->mpi_send);
}

bool check_mpi_Isend_conflicts(Module &M) {

  if (mpi_func->mpi_Ibsend != nullptr || mpi_func->mpi_Issend != nullptr ||
      mpi_func->mpi_Irsend != nullptr) {
    errs() << "This analysis does not cover the usage of any of Ib Ir or "
              "Issend operations. Replace them with another send mode like "
              "Isend instead\n";
    return false;
  }

  return check_conflicts(M, mpi_func->mpi_Isend);
}

bool check_mpi_Bsend_conflicts(Module &M) {
  return check_conflicts(M, mpi_func->mpi_Bsend);
}

bool check_mpi_Ssend_conflicts(Module &M) {
  // return check_conflicts(M,mpi_func->mpi_Ssend);
  // Ssend may not yield to conflicts regarding overtaking messages:
  // when Ssend returns: the receiver have begun execution of the matching recv
  // therefore further send operation may not overtake this one

  // this ssend can overtake another send ofc but this conflict will be handled
  // when the overtakend send is analyzed
  return false;
}

bool check_mpi_Rsend_conflicts(Module &M) {
  // return check_conflicts(M, mpi_func->mpi_Rsend);

  // standard:
  // A send operation that uses the ready mode has the same semantics as a
  // standard send operation, or a synchronous send operation; it is merely that
  // the sender provides additional information to the system (namely that a
  // matching receive is already posted), that can save some overhead. In a
  // correct program, therefore, a ready send couldbe replaced by a standard
  // send with no effect on the behavior of the program other than performance.

  // This means, the sender have started to execute the matching send, therefore
  // it has the same as Ssend.

  return false;
}

bool are_calls_conflicting(CallBase *orig_call, CallBase *conflict_call) {

  errs() << "\n";
  orig_call->dump();
  errs() << "potential conflict detected: ";
  conflict_call->dump();
  errs() << "\n";
  if (orig_call == conflict_call) {
    errs() << "Send is conflicting with itself, probably in a loop, if using "
              "different msg tags on each iteration this is safe nonetheless\n";
    return true;
  }

  // check communicator
  auto *comm1 = get_communicator(orig_call);
  auto *comm2 = get_communicator(conflict_call);

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
  auto *src1 = get_src(orig_call);
  auto *src2 = get_src(conflict_call);
  if (auto *s1 = dyn_cast<Constant>(src1)) {
    if (auto *s2 = dyn_cast<Constant>(src2)) {
      if (s1 != s2) {
        // different sources
        return false;
      }
    }
  } // otherwise, we have not proven that the src might be different

  // check tag
  auto *tag1 = get_tag(orig_call);
  auto *tag2 = get_tag(conflict_call);
  if (auto *t1 = dyn_cast<Constant>(tag1)) {
    if (auto *t2 = dyn_cast<Constant>(tag2)) {
      if (t1 != t2) {
        // different tags
        return false;
      }
    }
  } // otherwise, we have not proven that the tag is be different

  // cannot disprove conflict, have to assume it indeed relays on msg ordering
  return true;
}

Value *get_communicator(CallBase *mpi_call) {

  unsigned int total_num_args = 0;
  unsigned int communicator_arg_pos = 0;

  if (mpi_call->getCalledFunction() == mpi_func->mpi_send ||
      mpi_call->getCalledFunction() == mpi_func->mpi_Bsend ||
      mpi_call->getCalledFunction() == mpi_func->mpi_Ssend ||
      mpi_call->getCalledFunction() == mpi_func->mpi_Rsend) {
    total_num_args = 6;
    communicator_arg_pos = 5;
  } else if (mpi_call->getCalledFunction() == mpi_func->mpi_Isend) {
    total_num_args = 7;
    communicator_arg_pos = 5;
  } else {
    errs() << mpi_call->getCalledFunction()->getName()
           << ": This MPI function is currently not supported\n";
    assert(false);
  }

  assert(mpi_call->getNumArgOperands() == total_num_args);

  return mpi_call->getArgOperand(communicator_arg_pos);
}

Value *get_src(CallBase *mpi_call) {

  unsigned int total_num_args = 0;
  unsigned int src_arg_pos = 0;

  if (mpi_call->getCalledFunction() == mpi_func->mpi_send ||
      mpi_call->getCalledFunction() == mpi_func->mpi_Bsend ||
      mpi_call->getCalledFunction() == mpi_func->mpi_Ssend ||
      mpi_call->getCalledFunction() == mpi_func->mpi_Rsend) {
    total_num_args = 6;
    src_arg_pos = 3;
  } else if (mpi_call->getCalledFunction() == mpi_func->mpi_Isend) {
    total_num_args = 7;
    src_arg_pos = 3;
  } else {
    errs() << mpi_call->getCalledFunction()->getName()
           << ": This MPI function is currently not supported\n";
    assert(false);
  }

  assert(mpi_call->getNumArgOperands() == total_num_args);

  return mpi_call->getArgOperand(src_arg_pos);
}

Value *get_tag(CallBase *mpi_call) {

  unsigned int total_num_args = 0;
  unsigned int tag_arg_pos = 0;

  if (mpi_call->getCalledFunction() == mpi_func->mpi_send ||
      mpi_call->getCalledFunction() == mpi_func->mpi_Bsend ||
      mpi_call->getCalledFunction() == mpi_func->mpi_Ssend ||
      mpi_call->getCalledFunction() == mpi_func->mpi_Rsend) {
    total_num_args = 6;
    tag_arg_pos = 4;
  } else if (mpi_call->getCalledFunction() == mpi_func->mpi_Isend) {
    total_num_args = 7;
    tag_arg_pos = 4;
  } else {
    errs() << mpi_call->getCalledFunction()->getName()
           << ": This MPI function is currently not supported\n";
    assert(false);
  }

  assert(mpi_call->getNumArgOperands() == total_num_args);

  return mpi_call->getArgOperand(tag_arg_pos);
}
