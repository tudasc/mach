#include "implementation_specific.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Module.h"

#include "llvm/Support/raw_ostream.h"
#include <mpi.h>

using namespace llvm;

ImplementationSpecifics::ImplementationSpecifics(Module &M) {

  // need it to use MPI_Type_size
  // MPI_Init(NULL, NULL);

  COMM_WORLD =
      ConstantInt::get(IntegerType::get(M.getContext(), 32), MPI_COMM_WORLD);
  ANY_TAG = ConstantInt::get(IntegerType::get(M.getContext(), 32), MPI_ANY_TAG);
  ANY_SOURCE =
      ConstantInt::get(IntegerType::get(M.getContext(), 32), MPI_ANY_SOURCE);
}
ImplementationSpecifics::~ImplementationSpecifics() {
  // MPI_Finalize();
}

int ImplementationSpecifics::get_size_of_mpi_type(llvm::Constant *type) {
  // TODO if DataType is no integer type, this will break...

  if (auto *i = dyn_cast<ConstantInt>(type)) {
    int mpi_type = i->getSExtValue();

    switch (mpi_type) {
    case MPI_CHAR:
    case MPI_SIGNED_CHAR:
    case MPI_UNSIGNED_CHAR:
      return sizeof(char);
      break;
    case MPI_SHORT:
    case MPI_UNSIGNED_SHORT:
      return sizeof(short int);
      break;
    case MPI_INT:
    case MPI_UNSIGNED:
      return sizeof(int);
      break;
    case MPI_LONG:
    case MPI_UNSIGNED_LONG:
      return sizeof(long int);
      break;
    case MPI_LONG_LONG:
      // case MPI_LONG_LONG_INT:
    case MPI_UNSIGNED_LONG_LONG:
      return sizeof(long long int);
      break;
    case MPI_FLOAT:
      return sizeof(float);
      break;
    case MPI_DOUBLE:
      return sizeof(double);
    case MPI_LONG_DOUBLE:
      return sizeof(long double);
    case MPI_WCHAR:
      return sizeof(wchar_t);
      break;
    case MPI_C_BOOL:
      return sizeof(bool);
      break;
    case MPI_INT8_T:
    case MPI_UINT8_T:
      return sizeof(int8_t);
      break;
    case MPI_INT16_T:
    case MPI_UINT16_T:
      return sizeof(int16_t);
      break;
    case MPI_INT32_T:
    case MPI_UINT32_T:
      return sizeof(int32_t);
      break;
    case MPI_INT64_T:
    case MPI_UINT64_T:
      return sizeof(int64_t);
      break;
    case MPI_C_COMPLEX:
      // case MPI_C_FLOAT_COMPLEX:
      return sizeof(float _Complex);
      break;
    case MPI_C_DOUBLE_COMPLEX:
      return sizeof(double _Complex);
      break;
    case MPI_C_LONG_DOUBLE_COMPLEX:
      return sizeof(long double _Complex);
      break;
    case MPI_BYTE:
      return 1;
      break;
    default:
      errs() << "Unknown MPI Type" << mpi_type << "\n";
      assert(false);
      break;
    }
  }

  else {
    errs() << "MPI_Type is not an integer in your MPI implementation, this is "
              "not supported";
    assert(false);
  }
}
