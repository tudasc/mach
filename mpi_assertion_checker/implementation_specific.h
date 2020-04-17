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

#ifndef MACH_IMPLEMENTATION_SPECIFIC_H_
#define MACH_IMPLEMENTATION_SPECIFIC_H_

#include "llvm/IR/Constant.h"
class ImplementationSpecifics {

public:
  ImplementationSpecifics(llvm::Module &M);
  ~ImplementationSpecifics();

  llvm::Constant *COMM_WORLD;
  llvm::Constant *ANY_SOURCE;
  llvm::Constant *ANY_TAG;

  int get_size_of_mpi_type(llvm::Constant *type);
};

// created and deleted in main
extern ImplementationSpecifics *mpi_implementation_specifics;

#endif /* MACH_IMPLEMENTATION_SPECIFIC_H_ */
