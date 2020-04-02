#include <mpi.h>
#include <stdio.h>

#define MSG_TAG 123

// It this allowed and deterministic?
// does it may deadlock?

int main() {
  int a = 0;

  MPI_Init(NULL, NULL);
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Message msg;

  switch (rank) {
  case 0:
    MPI_Mprobe(1, MSG_TAG, MPI_COMM_WORLD, &msg, MPI_STATUS_IGNORE);

    MPI_Recv(&a, 1, MPI_INT, 1, MSG_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    printf("a=%d\n", a);

    MPI_Mrecv(&a, 1, MPI_INT, &msg, MPI_STATUS_IGNORE);
    printf("a=%d\n", a);

    break;
  case 1:
    a = 1;
    MPI_Send(&a, 1, MPI_INT, 0, MSG_TAG, MPI_COMM_WORLD);
    a = 2;
    MPI_Send(&a, 1, MPI_INT, 0, MSG_TAG, MPI_COMM_WORLD);
    break;
  }
  MPI_Finalize();
}
