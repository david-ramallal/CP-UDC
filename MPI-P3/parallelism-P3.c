#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <mpi.h>

//#define DEBUG 1

#define N 1024

int main(int argc, char *argv[] ) {

  int i, j, numprocs, rank, *distribution, m, remainder, n, *disp, k;
  float matrix[N][N], *matrixVector;
  float vector[N];
  float result[N], tmpResult[N];
  struct timeval  tv1, tv2;
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
  }  

  k=0;
  for(i = 0; i < N; i++){
    for(j = 0; j < N; j++){
      matrixVector[k] = matrix[i][j];
      k++;
    }
  }

  distribution = malloc(sizeof(int)*numprocs);
  disp = malloc(sizeof(int)*numprocs);
  auxMtrx = malloc (sizeof(float)*N*N);
  auxRslt = malloc (sizeof(float)*N);

  MPI_Bcast(&vector, N, MPI_FLOAT, 0, MPI_COMM_WORLD);

  if(N % numprocs == 0){
    m = N / numprocs;
    for(i = 0; i < numprocs; i++){
      distribution[i] = m*N;
      disp[i] = m*N*i;
    }
  }else{
    n = N;
    remainder = 0;
    while(n % numprocs != 0){
      remainder++;
      n--;
    }
    m = n / numprocs;
    distribution[0] = (m + remainder)*N;
    disp[0] = 0;
    for(i = 1; i < numprocs; i++){
      distribution[i] = m*N;
      disp[i] = m*N*i;
    }
  }  

  MPI_Scatterv(&matrixVector, distribution, disp, MPI_FLOAT, auxMtrx, N*N, MPI_FLOAT, 0, MPI_COMM_WORLD);

  gettimeofday(&tv1, NULL);

  // for(i=0;i<N;i++) {
  //   result[i]=0;
  //   for(j=0;j<N;j++) {
  //     result[i] += matrix[i][j]*vector[j];
  //   }
  // }  

  for(i=rank + 1;i<N;i+=numprocs) {
    auxRslt[i]=0;
    for(j=0;j<N;j++) {
      auxRslt[i] += matrixVector[i*N + j]*vector[j];
    }
  }  

  gettimeofday(&tv2, NULL);

  //MPI_Barrier(MPI_COMM_WORLD);

  MPI_Gatherv(&auxRslt, N, MPI_FLOAT, &result, distribution, disp, MPI_FLOAT, 0, MPI_COMM_WORLD);
    
  int microseconds = (tv2.tv_usec - tv1.tv_usec)+ 1000000 * (tv2.tv_sec - tv1.tv_sec);

  /*Display result */
  if (rank == 0){
    for(i=0;i<N;i++) {
      printf(" %f \t ",result[i]);
    }
  } else {
    printf ("Time (seconds) = %lf\n", (double) microseconds/1E6);
  }    

  return 0;
}