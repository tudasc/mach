#include <mpi.h>
#include <stdio.h>

#define MSG_TAG_A 123
#define MSG_TAG_B 1234
#define N 1000

int main() {
  int a = 1;
  int b = 2;

  MPI_Init(NULL, NULL);
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  if (rank == 0) {
    MPI_Recv(&a, 1, MPI_INT, 1, MSG_TAG_A, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Recv(&b, 1, MPI_INT, 1, MSG_TAG_B, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  } else if (rank == 1) {
    MPI_Send(&a, 1, MPI_INT, 0, MSG_TAG_A, MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Send(&b, 1, MPI_INT, 0, MSG_TAG_B, MPI_COMM_WORLD);
  }
  MPI_Finalize();
}
