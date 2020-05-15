#include <assert.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define MSG_TAG_A 123
#define MSG_TAG_B 1234
#define N 1000

// at first glance: no conflict possible, as each iteration is seperated by an
// allreduce But actually there is a conflict, if size==1, the pred and next is
// the same, therefore different tags are required here

// true modulo and not only reminder
inline int mod(int a, int b) {
  int r = a % b;
  return r < 0 ? r + b : r;
}

// sort of a 1d stencil
// imitating a Halo exchange on each iteration
int main() {
  int *data;
  data = malloc(N * sizeof(int));
  assert(data != NULL);
  for (int i = 0; i < N; ++i) {
    data[i] = 0;
  }
  int done = 0;

  MPI_Init(NULL, NULL);
  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  if (rank == 0) {
    data[0] = 1;
    done = 1;
  }

  const int pre = mod(rank - 1, size);
  const int next = mod(rank + 1, size);
  printf("rank:%d pre:%d next:%d\n", rank, pre, next);

  // 2^30
  while (done < 1073741824) {
    for (int i = 1; i < N - 1; ++i) {
      data[i] = data[i - 1] + data[i + 1];
    }

    // a halo exchange
    if (rank % 2 == 0) {
      MPI_Send(&data[0], 1, MPI_INT, next, MSG_TAG_A, MPI_COMM_WORLD);
      MPI_Recv(&data[1], 1, MPI_INT, pre, MSG_TAG_A, MPI_COMM_WORLD,
               MPI_STATUS_IGNORE);

      MPI_Send(&data[N - 2], 1, MPI_INT, pre, MSG_TAG_B, MPI_COMM_WORLD);
      MPI_Recv(&data[N - 1], 1, MPI_INT, next, MSG_TAG_B, MPI_COMM_WORLD,
               MPI_STATUS_IGNORE);
    } else {

      MPI_Recv(&data[0], 1, MPI_INT, pre, MSG_TAG_B, MPI_COMM_WORLD,
               MPI_STATUS_IGNORE);
      MPI_Send(&data[1], 1, MPI_INT, next, MSG_TAG_B, MPI_COMM_WORLD);

      MPI_Recv(&data[N - 1], 1, MPI_INT, next, MSG_TAG_A, MPI_COMM_WORLD,
               MPI_STATUS_IGNORE);
      MPI_Send(&data[N - 21], 1, MPI_INT, pre, MSG_TAG_A, MPI_COMM_WORLD);
    }

    // allreduce e.g. to get wether to terminate
    MPI_Allreduce(MPI_IN_PLACE, &done, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

    if (rank == 0) {
      printf("%d\n", done);
    }
  }

  if (rank == size - 1) {
    printf("Got last rank has %d as first data\n", data[0]);
  }

  free(data);

  MPI_Finalize();
}
