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

#include "function_coverage.h"
#include "mpi_functions.h"

#include "llvm/IR/InstIterator.h"

#include "debug.h"

using namespace llvm;

FunctionMetadata::FunctionMetadata(const llvm::TargetLibraryInfo *TLI,
                                   llvm::Module &M) {

  assert(mpi_func != nullptr);

  LibFunc libF;
  for (auto &F : M) {

    // if one of this attr: no conflict possible
    bool has_certain_attr = F.hasFnAttribute(Attribute::NoReturn);

    if (has_certain_attr || F.isIntrinsic() || TLI->getLibFunc(F, libF) ||
        is_mpi_function(&F)) {
      // errs() << F.getName() << " is part of stdlibs or an MPI call\n";
      // we consider MPi calls themselves to not include any forther mpi
      // commands, as they will be handeled differently form other function
      // calls anyway unknown, has mpi, has sync, may conflict
      auto info = std::make_tuple(false, false, false, false);
      const auto status =
          this->function_metadata.insert(std::make_pair(&F, info));
      assert(status.second && "Successfully inserted into map");

    } else {
      // errs() << F.getName() << " is user defined (or another library )\n";
      // to be analyezed if opposite holds:
      bool unknown = false;
      bool has_mpi = false;
      bool has_sync = false;
      bool may_conflict = false;

      if (F.isDeclaration()) {
        // not defined in this module
        unknown = true;
      } else {
        for (inst_iterator I = inst_begin(&F), E = inst_end(&F); I != E; ++I) {

          if (auto *call = dyn_cast<CallBase>(&*I)) {
            if (is_mpi_call(call)) {
              has_mpi = true;
              if (mpi_func->sync_functions.find(call->getCalledFunction()) !=
                  mpi_func->sync_functions.end()) {
                has_sync = true;
              }
              if (mpi_func->conflicting_functions.find(
                      call->getCalledFunction()) !=
                  mpi_func->conflicting_functions.end()) {
                may_conflict = true;
              }
            }
          }
        }
      }

      Debug(errs() << F.getName() << ":unknown: " << unknown
                   << " has_mpi: " << has_mpi << " will_sync: " << has_sync
                   << " may_conflict" << may_conflict << "\n";);
      auto info = std::make_tuple(unknown, has_mpi, has_sync, may_conflict);
      const auto status =
          this->function_metadata.insert(std::make_pair(&F, info));
      assert(status.second && "Successfully inserted into map");
    }
  }
}

bool FunctionMetadata::has_mpi(llvm::Function *F) {

  auto search = this->function_metadata.find(F);
  if (search != function_metadata.end()) {
    auto info = search->second;

    // unknown || has_mpi
    return std::get<0>(info) || std::get<1>(info);
  } else {
    // no analysis for this function present: assuming the worst
    return true;
  }
}
bool FunctionMetadata::may_conflict(llvm::Function *F) {
  auto search = this->function_metadata.find(F);
  if (search != function_metadata.end()) {
    auto info = search->second;
    // unknown || may_conflict
    return std::get<0>(info) || std::get<3>(info);
  } else {
    // no analysis for this function present: assuming the worst
    return true;
  }
}
bool FunctionMetadata::will_sync(llvm::Function *F) {
  auto search = this->function_metadata.find(F);
  if (search != function_metadata.end()) {
    auto info = search->second;
    // ! unknown && will_sync
    return !std::get<0>(info) && std::get<2>(info);
  } else {
    // no analysis for this function present: assuming the worst
    return false;
  }
}

bool FunctionMetadata::is_unknown(llvm::Function *F) {
  auto search = this->function_metadata.find(F);
  if (search != function_metadata.end()) {
    auto info = search->second;
    // unknown
    return std::get<0>(info);
  } else {
    // no analysis for this function present: unknown
    return true;
  }
}
