#include <assert.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define MSG_TAG 123
#define N 1000

// this is standard conform with no deadlock, and the isend msgs may overtake
int main() {
  int a = 1;
  int b = 2;

  MPI_Request req[2];

  MPI_Init(NULL, NULL);
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  switch (rank) {
  case 0:
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Recv(&a, 1, MPI_INT, 1, MSG_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(&b, 1, MPI_INT, 1, MSG_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    break;
  case 1:
    MPI_Isend(&a, 1, MPI_INT, 0, MSG_TAG, MPI_COMM_WORLD, req[0]);
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Isend(&b, 1, MPI_INT, 0, MSG_TAG, MPI_COMM_WORLD, req[1]);
    MPI_Waitall(2, &req, MPI_STATUSES_IGNORE);
    break;
  }

  MPI_Finalize();
}
