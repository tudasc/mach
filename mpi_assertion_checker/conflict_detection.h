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

#ifndef MACH_CONFLICT_DETECTION_H_
#define MACH_CONFLICT_DETECTION_H_

#include "llvm/IR/InstrTypes.h"

#include <vector>

std::vector<std::pair<llvm::CallBase *, llvm::CallBase *>>
check_mpi_recv_conflicts(llvm::Module &M);

std::vector<std::pair<llvm::CallBase *, llvm::CallBase *>>
check_mpi_send_conflicts(llvm::Module &M);

llvm::Value *get_communicator(llvm::CallBase *mpi_call);
llvm::Value *get_src(llvm::CallBase *mpi_call, bool is_send);
llvm::Value *get_tag(llvm::CallBase *mpi_call, bool is_send);

#endif /* MACH_CONFLICT_DETECTION_H_ */
