/*
 * conflict_detection.cpp
 *
 *  Created on: 25.03.2020
 *      Author: Tim Jammer
 */

#include "conflict_detection.h"
#include "mpi_functions.h"

using namespace llvm;

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
        // TODO we need to exclude any MPI_ANY_SOURCE here
        errs() << "If you use MPI_ANY_SOURCE in any MPI call, this analysis "
                  "might be screwed, as this is currently not supported\n";
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
        // TODO we need to exclude any MPI_ANY_TAG here
        errs() << "If you use MPI_ANY_TAG in any MPI call, this analysis might "
                  "be screwed, as this is currently not supported\n";
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

  if (mpi_call->getCalledFunction() == mpi_func->mpi_recv) {
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

  if (mpi_call->getCalledFunction() == mpi_func->mpi_recv) {
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

  if (mpi_call->getCalledFunction() == mpi_func->mpi_recv) {
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
