#include <assert.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MSG_TAG 123
#define N 1000

// no dependency, this in nondeterministic according to the current standard
// anyway
// hard to find it statically

// process 0 is the master telling others what to do
int main() {

  int data;
  int done = 0;

  MPI_Status stat;
  MPI_Init(NULL, NULL);
  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  data = rank;

  if (rank == 0) {
    printf("%d ", data);
    for (int i = 1; i < N; ++i) {
      MPI_Recv(&data, 1, MPI_INT, MPI_ANY_SOURCE, MSG_TAG, MPI_COMM_WORLD,
               &stat);
      data = i;
      int from = stat.MPI_SOURCE;
      MPI_Send(&data, 1, MPI_INT, from, MSG_TAG, MPI_COMM_WORLD);
      printf("%d ", data);
    }
    data = -1;
    for (int i = 1; i < size; ++i) {
      // tell others to stop
      MPI_Recv(&data, 1, MPI_INT, MPI_ANY_SOURCE, MSG_TAG, MPI_COMM_WORLD,
               &stat);
      data = -1;
      int from = stat.MPI_SOURCE;
      MPI_Send(&data, 1, MPI_INT, from, MSG_TAG, MPI_COMM_WORLD);
    }
  } else {

    while (!done) {
      // ask master for next task
      MPI_Send(&data, 1, MPI_INT, 0, MSG_TAG, MPI_COMM_WORLD);
      MPI_Recv(&data, 1, MPI_INT, 0, MSG_TAG, MPI_COMM_WORLD,
               MPI_STATUS_IGNORE);
      if (data == -1)
        done = 1;
      else
        usleep(10); // work
    }
  }

  MPI_Finalize();
}
