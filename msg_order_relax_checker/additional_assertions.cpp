/*
 * additional_assertions.cpp
 *
 *  Created on: 03.04.2020
 *      Author: Tim Jammer
 */
#include "additional_assertions.h"
#include "conflict_detection.h"
#include "implementation_specific.h"
#include "mpi_functions.h"

using namespace llvm;

// Todo may use some refactoring to avoid code duplication here
// Todo may do analysis on a per communicator basis
bool check_any_tag_for_function(Function *f) {
  if (f == nullptr) {
    return true;
  } else {
    for (auto *u : f->users()) {

      if (auto *call = dyn_cast<CallBase>(u)) {
        if (call->getCalledFunction() == f) {
          auto *tag = get_tag(call, false);
          if (auto *c = dyn_cast<Constant>(tag)) {
            if (c == mpi_implementation_specifics->ANY_TAG) {
              return false;
            }
          }
          // else it is not any tag, I see it as miss usage it a non constant
          // value is (probably by surprise) any tag
        }
      }
    }
  }
  // not found any any tag
  return true;
}

bool check_any_source_for_function(Function *f) {
  if (f == nullptr) {
    return true;
  } else {
    for (auto *u : f->users()) {

      if (auto *call = dyn_cast<CallBase>(u)) {
        if (call->getCalledFunction() == f) {
          auto *src = get_src(call, false);
          if (auto *c = dyn_cast<Constant>(src)) {
            if (c == mpi_implementation_specifics->ANY_SOURCE) {
              return false;
            }
          }
        }
      }
    }
  }
  // not found any any source
  return true;
}

// checks wether the mpi_assert_no_any_tag flag may be set
bool check_no_any_tag(llvm::Module &M) {
  bool result = true;
  result = result && check_any_tag_for_function(mpi_func->mpi_recv);
  result = result && check_any_tag_for_function(mpi_func->mpi_Irecv);
  // TODO add mpi probe, iprobe, mprobe, improbe,

  return result;
}

// checks wether the mpi_assert_no_any_source flag may be set
bool check_no_any_source(llvm::Module &M) {
  bool result = true;
  result = result && check_any_source_for_function(mpi_func->mpi_recv);
  result = result && check_any_source_for_function(mpi_func->mpi_Irecv);
  // TODO add mpi probe, iprobe, mprobe, improbe,

  return result;
}

// TODO also have a look at mpi_assert_exact_length
