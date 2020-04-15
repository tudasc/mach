#ifndef MSG_ORDER_RELAX_CHECKER_IMPLEMENTATION_SPECIFIC_H_
#define MSG_ORDER_RELAX_CHECKER_IMPLEMENTATION_SPECIFIC_H_

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

#endif /* MSG_ORDER_RELAX_CHECKER_IMPLEMENTATION_SPECIFIC_H_ */
