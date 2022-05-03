#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <mpi.h>

#define DEBUG 0

#define N 1024

int main(int argc, char *argv[] ) {

  int i, j, numprocs, rank, *distribution, m, remainder = 0, n, *disp, *reception, *dispRcv, k;
  float matrix[N][N], *matrixVector;
  float vector[N];
  float result[N];
  struct timeval tv1, tv2, bcast1, bcast2, gath1, gath2, scatt1, scatt2;
  float *auxMtrx, *auxRslt;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  matrixVector = malloc(sizeof(float) * N * N);

  if(rank == 0){
    /* Initialize Matrix and Vector */
    for(i=0;i<N;i++) {
      vector[i] = i;
      for(j=0;j<N;j++) {
        matrix[i][j] = i+j;
      }
    }

    k=0;
    for(i = 0; i < N; i++){
      for(j = 0; j < N; j++){
        matrixVector[k] = matrix[i][j];
        k++;
      }
    }
  }    

  distribution = malloc(sizeof(int)*numprocs);
  reception = malloc(sizeof(int)*numprocs);
  disp = malloc(sizeof(int)*numprocs);
  dispRcv = malloc(sizeof(int)*numprocs);
  auxRslt = malloc (sizeof(float)*N);

  gettimeofday(&bcast1, NULL);

  MPI_Bcast(&vector, N, MPI_FLOAT, 0, MPI_COMM_WORLD);

  gettimeofday(&bcast2, NULL);

  if(N % numprocs == 0){
    m = N / numprocs;
    for(i = 0; i < numprocs; i++){
      distribution[i] = m*N;
      disp[i] = m*N*i;
    }
  }else{
    n = N;
    while(n % numprocs != 0){
      remainder++;
      n--;
    }
    m = n / numprocs;
    distribution[0] = (m + remainder)*N;
    disp[0] = 0;
    for(i = 1; i < numprocs; i++){
      distribution[i] = m*N;
      disp[i] = (m+remainder)*N + m*N*(i-1);
    }
  }

  auxMtrx = malloc (sizeof(float)*(m + remainder)*N);

  gettimeofday(&scatt1, NULL);

  if(rank == 0){
    MPI_Scatterv(matrixVector, distribution, disp, MPI_FLOAT, auxMtrx, (m+remainder)*N, MPI_FLOAT, 0, MPI_COMM_WORLD);
  }else
    MPI_Scatterv(matrixVector, distribution, disp, MPI_FLOAT, auxMtrx, m*N, MPI_FLOAT, 0, MPI_COMM_WORLD);

  gettimeofday(&scatt2, NULL);


  gettimeofday(&tv1, NULL);

  if(rank == 0){
    for(i=0;i<(m + remainder);i++) {
      auxRslt[i]=0;
      for(j=0;j<N;j++) {
        auxRslt[i] += auxMtrx[i*N + j]*vector[j];
      }
    }
  }
  else{
    for(i=0;i<m;i++) {
      auxRslt[i]=0;
      for(j=0;j<N;j++) {
        auxRslt[i] += auxMtrx[i*N + j]*vector[j];
      }
    }
  }

  if(N % numprocs == 0){
    for(i = 0; i < numprocs; i++){
      reception[i] = m;
      dispRcv[i] = m*i;
    }
  }else{
    reception[0] = (m + remainder);
    dispRcv[0] = 0;
    for(i = 1; i < numprocs; i++){
      reception[i] = m;
      dispRcv[i] = (m+remainder) + m*(i-1);
    }
  }

  gettimeofday(&tv2, NULL);

  gettimeofday(&gath1, NULL);

  if(rank == 0){
    MPI_Gatherv(auxRslt, m+remainder, MPI_FLOAT, &result, reception, dispRcv, MPI_FLOAT, 0, MPI_COMM_WORLD);
  }else
    MPI_Gatherv(auxRslt, m, MPI_FLOAT, &result, reception, dispRcv, MPI_FLOAT, 0, MPI_COMM_WORLD);

  gettimeofday(&gath2, NULL);
    
  int microseconds = (tv2.tv_usec - tv1.tv_usec)+ 1000000 * (tv2.tv_sec - tv1.tv_sec);

  int msBcast = (bcast2.tv_usec - bcast1.tv_usec)+ 1000000 * (bcast2.tv_sec - bcast1.tv_sec);
  int msScatt = (scatt2.tv_usec - scatt1.tv_usec)+ 1000000 * (scatt2.tv_sec - scatt1.tv_sec);
  int msGath = (gath2.tv_usec - gath1.tv_usec)+ 1000000 * (gath2.tv_sec - gath1.tv_sec);

  int commTime = msBcast + msScatt + msGath;

  /*
  To display the result -> DEBUG = 1
  To display the times -> DEBUG = 0
  */

  if(DEBUG){
    if(rank == 0){
      for(i=0;i<N;i++) {
        printf(" %f \t ",result[i]);
      }
    }
  }else{
    MPI_Barrier(MPI_COMM_WORLD);
    printf("Computation time (seconds) = %lf (process %d)\n", (double) microseconds/1E6, rank);
    printf("Communication time (seconds) = %lf (process %d)\n", (double) commTime/1E6, rank);
  }  

  MPI_Finalize(); 

  return 0;
}