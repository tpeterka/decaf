//---------------------------------------------------------------------------
//
// Simple example using active and passive targets
//
// Matthieu Dreher
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------


#include <mpi.h>
#include <stdio.h>

int main()
{
    MPI_Init(NULL, NULL);

    int rank, size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if(size != 3)
    {
        fprintf(stderr, "Usage: mpirun -n 3 simple_onesided\n");
        MPI_Abort(MPI_COMM_WORLD, 0);
    }

    // Creation of the window and is associated memory
    MPI_Win win_test;
    int my_flag = rank;
    MPI_Win_create(&my_flag, 1, sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &win_test);

    int flag1 = -1, flag2 = -1;

    // Active mode
    MPI_Win_fence(MPI_MODE_NOPUT | MPI_MODE_NOPRECEDE, win_test);

    MPI_Get(&flag1, 1, MPI_INT, (rank + 1) % size, 0, 1, MPI_INT, win_test);
    MPI_Get(&flag2, 1, MPI_INT, (rank + 2) % size, 0, 1, MPI_INT, win_test);

    MPI_Win_fence(MPI_MODE_NOSTORE, win_test);
    fprintf(stderr,"Active mode: My rank: %i, next rank: %i, following rank: %i\n", rank, flag1, flag2);

    // Passive mode
    flag1 = -1;
    flag2 = -1;

    MPI_Win_lock(MPI_LOCK_SHARED, (rank + 1) % size, 0, win_test);
    MPI_Get(&flag1, 1, MPI_INT, (rank + 1) % size, 0, 1, MPI_INT, win_test);
    MPI_Win_unlock((rank + 1) % size, win_test);

    MPI_Win_lock(MPI_LOCK_SHARED, (rank + 2) % size, 0, win_test);
    MPI_Get(&flag2, 1, MPI_INT, (rank + 2) % size, 0, 1, MPI_INT, win_test);
    MPI_Win_unlock((rank + 2) % size, win_test);

    fprintf(stderr,"Passive mode: My rank: %i, next rank: %i, following rank: %i\n", rank, flag1, flag2);

    MPI_Win_free(&win_test);
    MPI_Finalize();
}
