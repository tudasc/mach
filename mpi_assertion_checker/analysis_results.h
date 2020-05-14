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

#ifndef MACH_ANALYSIS_RESULTS_H
#define MACH_ANALYSIS_RESULTS_H

#include "llvm/Analysis/AliasAnalysis.h"
#include <llvm/Analysis/LoopInfo.h>

// will be managed by main

// result of Alias analysis
extern std::map<llvm::Function *, llvm::AliasAnalysis *> AA;
// result of Loop Analysis
extern std::map<llvm::Function *, llvm::LoopInfo *> LI;

#endif
