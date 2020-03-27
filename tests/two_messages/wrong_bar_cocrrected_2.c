#include <assert.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define MSG_TAG 123
#define N 1000

// this is standard conform with no deadlock, and the buffered msgs may overtake
int main() {
  int a = 1;
  int b = 2;

  int bufer_size = (sizeof(int) + MPI_BSEND_OVERHEAD) * 2;
  void *sendbuffer = malloc(bufer_size);

  MPI_Buffer_attach(sendbuffer, bufer_size);

  MPI_Init(NULL, NULL);
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  switch (rank) {
  case 0:
    MPI_Recv(&a, 1, MPI_INT, 1, MSG_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Recv(&b, 1, MPI_INT, 1, MSG_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    break;
  case 1:
    MPI_Bsend(&a, 1, MPI_INT, 0, MSG_TAG, MPI_COMM_WORLD);
    MPI_Bsend(&b, 1, MPI_INT, 0, MSG_TAG, MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    break;
  }

  void *detach_addr = NULL;
  int detach_size = 0;
  MPI_Buffer_detach(
      &detach_addr,
      &detach_size); // here all the sending operations must complete
  assert(detach_addr == sendbuffer);
  assert(detach_size == bufer_size);

  free(sendbuffer);
  MPI_Finalize();
}
