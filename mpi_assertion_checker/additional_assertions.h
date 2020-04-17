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

#ifndef MACH_ADDITIONAL_ASSERTIONS_H_
#define MACH_ADDITIONAL_ASSERTIONS_H_

#include "llvm/IR/Module.h"

bool check_no_any_tag(llvm::Module &M);
bool check_no_any_source(llvm::Module &M);

bool check_exact_length(llvm::Module &M);

#endif /* MACH_ADDITIONAL_ASSERTIONS_H_ */
