#include <mpi.h>
#include <stdio.h>

#define MSG_TAG 123
#define N 1000

int main() {
  int a = 1;
  int b = 2;
  int c = 3;

  MPI_Request req[3];

  MPI_Init(NULL, NULL);
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  switch (rank) {
  case 0:
    MPI_Recv(&a, 1, MPI_INT, 1, MSG_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(&a, 1, MPI_INT, 2, MSG_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Recv(&b, 1, MPI_INT, 1, MSG_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    break;
  case 1:
    MPI_Isend(&a, 1, MPI_INT, 0, MSG_TAG, MPI_COMM_WORLD, &req[1]); // *
    MPI_Isend(&a, 1, MPI_INT, 2, MSG_TAG, MPI_COMM_WORLD, &req[0]);
    MPI_Waitall(1, req, MPI_STATUSES_IGNORE);
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Isend(&c, 1, MPI_INT, 0, MSG_TAG, MPI_COMM_WORLD,
              &req[2]); // conflicting may overtake *
    MPI_Wait(&req[2], MPI_STATUS_IGNORE);
    break;

  case 2:
    MPI_Send(&a, 1, MPI_INT, 0, MSG_TAG, MPI_COMM_WORLD);
    MPI_Recv(&a, 1, MPI_INT, 1, MSG_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Barrier(MPI_COMM_WORLD);
    break;
  }
  MPI_Finalize();
}
