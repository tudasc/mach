#include <mpi.h>
#include <stdio.h>

#define MSG_TAG 123
#define N 1000

// this might incur a conflict, as there is no guarantee,
// that 0 have started any operation when 1 finishes, therefore (very unlikely
// indeed) the msgs might overtake according to the standard this even applies
// for an allreduce see standard section 5.13 for more examples on that

int main() {
  int a = 1;
  int b = 2;
  int reduce = 1;

  MPI_Init(NULL, NULL);
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  if (rank == 0) {
    MPI_Recv(&a, 1, MPI_INT, 1, MSG_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Reduce(&reduce, NULL, 1, MPI_INT, MPI_SUM, 1, MPI_COMM_WORLD);
    MPI_Recv(&b, 1, MPI_INT, 1, MSG_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  } else if (rank == 1) {
    MPI_Send(&a, 1, MPI_INT, 0, MSG_TAG, MPI_COMM_WORLD);
    MPI_Reduce(&reduce, MPI_IN_PLACE, 1, MPI_INT, MPI_SUM, 1, MPI_COMM_WORLD);
    MPI_Send(&b, 1, MPI_INT, 0, MSG_TAG, MPI_COMM_WORLD);
  }
  MPI_Finalize();
}
