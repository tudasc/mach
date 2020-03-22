#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/InstrTypes.h" 
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Type.h"
#include "llvm/ADT/StringRef.h" 
#include "llvm/IR/DerivedTypes.h"
#include "llvm/ADT/APInt.h"
#include "llvm/IR/Instruction.h"

//#include <mpi.h>
#include <cstring>
#include <utility>

#include "mpi_functions.h"

using namespace llvm;

//declare dso_local i32 @MPI_Init(i32*, i8***) #1
//declare dso_local i32 @MPI_Comm_rank(i32, i32*) #1
//declare dso_local i32 @MPI_Recv(i8*, i32, i32, i32, i32, i32, %struct.MPI_Status*) #1
//declare dso_local i32 @MPI_Send(i8*, i32, i32, i32, i32, i32) #1
//declare dso_local i32 @MPI_Finalize() #1


namespace 
{
    struct MSGOrderRelaxCheckerPass : public ModulePass
    {
        static char ID;
        MSGOrderRelaxCheckerPass() : ModulePass(ID) {}

        //Pass starts here
        virtual bool runOnModule(Module &M) 
        {

        	struct mpi_functions * mpi_func = get_used_mpi_functions(M);
        	if (! is_mpi_used(mpi_func)){
        		// nothing to do for non mpi applicatiopns
        		free (mpi_func);
        		return false;
        	}
        	errs() << "Successfully executed the example pass\n\n";
        	free (mpi_func);
            //M.dump();
            return false;
        }
    };
}


char MSGOrderRelaxCheckerPass::ID = 42;

// Automatically enable the pass.
// http://adriansampson.net/blog/clangpass.html
static void registerExperimentPass(const PassManagerBuilder &, legacy::PassManagerBase &PM) 
{
    PM.add(new MSGOrderRelaxCheckerPass());
}

static RegisterStandardPasses
    RegisterMyPass(PassManagerBuilder::EP_ModuleOptimizerEarly,
                   registerExperimentPass);

static RegisterStandardPasses
    RegisterMyPass0(PassManagerBuilder::EP_EnabledOnOptLevel0,
                   registerExperimentPass);
