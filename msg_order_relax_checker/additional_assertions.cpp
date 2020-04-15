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

#include "additional_assertions.h"
#include "conflict_detection.h"
#include "implementation_specific.h"
#include "mpi_functions.h"

#include "llvm/IR/InstrTypes.h"
#include "llvm/Support/raw_ostream.h"

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

Value *get_type(CallBase *mpi_call, bool is_send) {

  unsigned int total_num_args = 0;
  unsigned int type_arg_pos = 0;

  if (mpi_call->getCalledFunction() == mpi_func->mpi_send ||
      mpi_call->getCalledFunction() == mpi_func->mpi_Bsend ||
      mpi_call->getCalledFunction() == mpi_func->mpi_Ssend ||
      mpi_call->getCalledFunction() == mpi_func->mpi_Rsend) {
    assert(is_send);
    total_num_args = 6;
    type_arg_pos = 2;
  } else if (mpi_call->getCalledFunction() == mpi_func->mpi_Isend) {
    assert(is_send);
    total_num_args = 7;
    type_arg_pos = 2;
  } else if (mpi_call->getCalledFunction() == mpi_func->mpi_recv ||
             mpi_call->getCalledFunction() == mpi_func->mpi_Irecv) {
    assert(!is_send);
    total_num_args = 7;
    type_arg_pos = 2;
  } else if (mpi_call->getCalledFunction() == mpi_func->mpi_Sendrecv) {
    total_num_args = 12;
    if (is_send)
      type_arg_pos = 2;
    else
      type_arg_pos = 7;
  } else {
    errs() << mpi_call->getCalledFunction()->getName()
           << ": This MPI function is currently not supported\n";
    assert(false);
  }

  assert(mpi_call->getNumArgOperands() == total_num_args);

  return mpi_call->getArgOperand(type_arg_pos);
}

Value *get_count(CallBase *mpi_call, bool is_send) {

  unsigned int total_num_args = 0;
  unsigned int count_arg_pos = 0;

  if (mpi_call->getCalledFunction() == mpi_func->mpi_send ||
      mpi_call->getCalledFunction() == mpi_func->mpi_Bsend ||
      mpi_call->getCalledFunction() == mpi_func->mpi_Ssend ||
      mpi_call->getCalledFunction() == mpi_func->mpi_Rsend) {
    assert(is_send);
    total_num_args = 6;
    count_arg_pos = 1;
  } else if (mpi_call->getCalledFunction() == mpi_func->mpi_Isend) {
    assert(is_send);
    total_num_args = 7;
    count_arg_pos = 1;
  } else if (mpi_call->getCalledFunction() == mpi_func->mpi_recv ||
             mpi_call->getCalledFunction() == mpi_func->mpi_Irecv) {
    assert(!is_send);
    total_num_args = 7;
    count_arg_pos = 1;
  } else if (mpi_call->getCalledFunction() == mpi_func->mpi_Sendrecv) {
    total_num_args = 12;
    if (is_send)
      count_arg_pos = 1;
    else
      count_arg_pos = 6;
  } else {
    errs() << mpi_call->getCalledFunction()->getName()
           << ": This MPI function is currently not supported\n";
    assert(false);
  }

  assert(mpi_call->getNumArgOperands() == total_num_args);

  return mpi_call->getArgOperand(count_arg_pos);
}

std::vector<std::tuple<Value *, Value *, int>>
get_lengths_for_function(Function *F, bool is_send) {

  if (F == nullptr) {
    return {};
  }

  std::vector<std::tuple<Value *, Value *, int>> result;
  for (auto *u : F->users()) {
    if (auto *call = dyn_cast<CallBase>(u)) {
      if (call->getCalledFunction() == F) {
        if (auto *type = dyn_cast<Constant>(get_type(call, is_send))) {

          int size = mpi_implementation_specifics->get_size_of_mpi_type(type);
          result.push_back(std::make_tuple(get_tag(call, is_send),
                                           get_count(call, is_send), size));

        } else {
          errs() << "Using a user defined type: could not detect "
                    "assert_exact_length\n";
          // insert a conflicting pair of values
          result.push_back(std::make_tuple(call, call, 1));
          result.push_back(std::make_tuple(call, call, 2));
        }
      }
    }
  }

  return result;
}

// ! true if no conflict false if conflict
bool check_lengths_for_conflicts(
    Module &M, std::vector<std::tuple<Value *, Value *, int>> sizes) {
  std::map<Value *, std::pair<Value *, int>> length_map;

  // we need a different llvm Value for all non constant tags
  // float is used, so we dont accidentally collide with constant tag
  auto non_constant_tag =
      Constant::getNullValue(Type::getFloatTy(M.getContext()));

  for (auto entry : sizes) {

    auto tag = std::get<0>(entry);
    auto count = std::get<1>(entry);
    auto size = std::get<2>(entry);

    if (!isa<Constant>(tag)) {
      tag = non_constant_tag;
    }

    if (length_map.find(tag) != length_map.end()) {
      // in map

      auto in_map = length_map.at(tag);

      auto compare_count = std::get<0>(in_map);
      auto compare_size = std::get<1>(in_map);

      if (count != compare_count || size != compare_size) {
        // found conflict
        return false;
      }

    } else { // not in map: insert
      length_map.insert(std::make_pair(tag, std::make_pair(count, size)));
    }
  }

  // not found conflict
  return true;
}

// TODO also have a look at mpi_assert_exact_length
bool check_exact_length(llvm::Module &M) {

  // list of Tag, count,type_size for all sends/recvs
  std::vector<std::tuple<Value *, Value *, int>> sizes;
  // maps each tag to a msg size

  auto tmp = get_lengths_for_function(mpi_func->mpi_send, true);
  sizes.insert(sizes.end(), tmp.begin(), tmp.end());
  tmp = get_lengths_for_function(mpi_func->mpi_Isend, true);
  sizes.insert(sizes.end(), tmp.begin(), tmp.end());
  tmp = get_lengths_for_function(mpi_func->mpi_Bsend, true);
  sizes.insert(sizes.end(), tmp.begin(), tmp.end());
  tmp = get_lengths_for_function(mpi_func->mpi_Rsend, true);
  sizes.insert(sizes.end(), tmp.begin(), tmp.end());
  tmp = get_lengths_for_function(mpi_func->mpi_Ssend, true);
  sizes.insert(sizes.end(), tmp.begin(), tmp.end());
  tmp = get_lengths_for_function(mpi_func->mpi_Ibsend, true);
  sizes.insert(sizes.end(), tmp.begin(), tmp.end());
  tmp = get_lengths_for_function(mpi_func->mpi_Irsend, true);
  sizes.insert(sizes.end(), tmp.begin(), tmp.end());
  tmp = get_lengths_for_function(mpi_func->mpi_Issend, true);
  sizes.insert(sizes.end(), tmp.begin(), tmp.end());
  tmp = get_lengths_for_function(mpi_func->mpi_Sendrecv, true);
  sizes.insert(sizes.end(), tmp.begin(), tmp.end());

  // recv
  tmp = get_lengths_for_function(mpi_func->mpi_recv, false);
  sizes.insert(sizes.end(), tmp.begin(), tmp.end());
  tmp = get_lengths_for_function(mpi_func->mpi_Irecv, false);
  sizes.insert(sizes.end(), tmp.begin(), tmp.end());
  tmp = get_lengths_for_function(mpi_func->mpi_Sendrecv, false);
  sizes.insert(sizes.end(), tmp.begin(), tmp.end());

  return check_lengths_for_conflicts(M, sizes);
}
