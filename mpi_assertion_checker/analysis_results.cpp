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

#include "mpi_functions.h"

#include "llvm/Pass.h"

#include "analysis_results.h"

using namespace llvm;

RequiredAnalysisResults::RequiredAnalysisResults(Pass *parent_pass) {

  assertion_checker_pass = parent_pass;

  assert(mpi_func != nullptr &&
         "The search for MPI functions should be made first");

  // yust give it any function, the Function is not used at all
  // dont know why the api has changed here...
  TLI = &assertion_checker_pass->getAnalysis<TargetLibraryInfoWrapperPass>()
             .getTLI(*mpi_func->mpi_init);

  current_LI_function = nullptr;
  current_SE_function = nullptr;
  current_AA_function = nullptr;
  current_LI = nullptr;
  current_SE = nullptr;
  current_AA = nullptr;
}

llvm::AAResults *RequiredAnalysisResults::getAAResults(llvm::Function *f) {
  if (current_AA_function != f) {
    current_AA_function = f;
    current_AA = &assertion_checker_pass->getAnalysis<AAResultsWrapperPass>(*f)
                      .getAAResults();
  }

  return current_AA;
}

llvm::LoopInfo *RequiredAnalysisResults::getLoopInfo(llvm::Function *f) {
  if (current_LI_function != f) {
    current_LI_function = f;
    current_LI = &assertion_checker_pass->getAnalysis<LoopInfoWrapperPass>(*f)
                      .getLoopInfo();
  }

  return current_LI;
}
llvm::ScalarEvolution *RequiredAnalysisResults::getSE(llvm::Function *f) {
  if (current_SE_function != f) {
    current_SE_function = f;
    current_SE =
        &assertion_checker_pass->getAnalysis<ScalarEvolutionWrapperPass>(*f)
             .getSE();
  }

  return current_SE;
}

llvm::TargetLibraryInfo *RequiredAnalysisResults::getTLI() { return TLI; }
