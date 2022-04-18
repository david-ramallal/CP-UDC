#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

int MPI_FlattreeColective(void *buff, void *recvbuff, int count, MPI_Datatype datatype, int root, MPI_Comm comm){

    int errorNum, i, numprocs, rank;
    double *buffInt, *recvbuffInt;
    MPI_Status status;
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    buffInt = (double *) buff;
    recvbuffInt = (double *) recvbuff;

    if(rank != root){
        errorNum = MPI_Send(buffInt, count, datatype, root, 0, comm);
        if(errorNum != MPI_SUCCESS)
            return errorNum;       
    }else{
        for(i = 0; i < numprocs; i++){
            if(i != root){
                errorNum = MPI_Recv(recvbuffInt, 1, MPI_DOUBLE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                *buffInt += *recvbuffInt;
                if(errorNum != MPI_SUCCESS)
                    return errorNum;
            }
            
        }
    }

    return MPI_SUCCESS;
             
}

int MPI_BinomialBcast(void *buff, int count, MPI_Datatype datatype, int root, MPI_Comm comm){
    
    int errorNum, i, numprocs, rank, parent, child;
    MPI_Status status;
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    for(i = 1; i <= ceil(log2(numprocs)); i++){
        if(rank < pow(2, i-1)){
            child = rank + pow(2, i-1);
            if(child < numprocs){
                errorNum = MPI_Send(buff, count, datatype, child, 0, comm);
                if(errorNum != MPI_SUCCESS)
                    return errorNum;
            }
        }else{
            if(rank < pow(2,i)){
                parent = rank - pow(2, i-1);
                errorNum = MPI_Recv(buff, count, datatype, parent, 0, comm, &status);
                if(errorNum != MPI_SUCCESS)
                    return errorNum;
            }
        }
    }
    return MPI_SUCCESS;
}

int MPI_BinomialColective(void *buff, int count, MPI_Datatype datatype, MPI_Comm comm){
    MPI_BinomialBcast(buff, count, datatype, 0, comm);
}


int main(int argc, char *argv[])
{
    int i, done = 0, n, count;
    double PI25DT = 3.141592653589793238462643;
    double pi, x, y, z, recvPi;
    int numprocs, rank;

    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);


    while (!done)
    {

        if(rank == 0){
            printf("Enter the number of points: (0 quits) \n");
            scanf("%d",&n);
        } 

        MPI_BinomialColective(&n, 1, MPI_INT, MPI_COMM_WORLD);

    
        if (n == 0) break;

        count = 0;  

        for (i = rank + 1; i <= n; i+=numprocs) {
            // Get the random numbers between 0 and 1
	    x = ((double) rand()) / ((double) RAND_MAX);
	    y = ((double) rand()) / ((double) RAND_MAX);

	    // Calculate the square root of the squares
	    z = sqrt((x*x)+(y*y));

	    // Check whether z is within the circle
	    if(z <= 1.0)
                count++;
        }
        pi = ((double) count/(double) n)*4.0;

        MPI_FlattreeColective(&pi, &recvPi, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

        if(rank == 0)
            printf("pi is approx. %.16f, Error is %.16f\n", pi, fabs(pi - PI25DT));

    }

    MPI_Finalize();
}