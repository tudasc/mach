#include <assert.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define MSG_TAG 123
#define N 1000

// irecv spanning over the barrier, may result in conflict
int main() {
  int a = 1;
  int b = 2;

  MPI_Request req[2];

  MPI_Init(NULL, NULL);
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  switch (rank) {
  case 0:

    MPI_Irecv(&a, 1, MPI_INT, 1, MSG_TAG, MPI_COMM_WORLD, &req[0]);
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Irecv(&b, 1, MPI_INT, 1, MSG_TAG, MPI_COMM_WORLD, &req[1]);
    MPI_Waitall(2, &req, MPI_STATUSES_IGNORE);
    break;
  case 1:
    MPI_Send(&a, 1, MPI_INT, 0, MSG_TAG, MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Send(&b, 1, MPI_INT, 0, MSG_TAG, MPI_COMM_WORLD);

    break;
  }

  MPI_Finalize();
}
