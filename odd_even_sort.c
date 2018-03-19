#include "mpi.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

int cmpfunc (const void * a, const void * b){
  if (*(double*)a > *(double*)b)
  return 1;
  else if (*(double*)a < *(double*)b)
  return -1;
  else
  return 0;
}

void Merge_low(double *localdata, double *tempdata, int I){
  /* Merge smallest parts of two equal sized arrays.
  */
  int local_i, temp_i, merge_i;
  double *mergearray;
  mergearray = (double *) malloc(I);
  local_i = temp_i = merge_i = 0;

  while (merge_i<I) {
    if (localdata[local_i]<tempdata[temp_i]) {
      mergearray[merge_i] = localdata[local_i];
      local_i++;
    }else{
      mergearray[merge_i] = tempdata[temp_i];
      temp_i++;
    }
    merge_i++;
  }
  // memcpy(localdata, mergearray, I*sizeof(double));
  for (int i = 0; i < I; i++) {
    localdata[i] = mergearray[i];
  }
  // free(mergearray);
  // free(tempdata);
}

void Merge_high(double *localdata, double *tempdata, int I){
  /* Merge largest parts of two equal sized arrays.
  */
  int local_i, temp_i, merge_i;
  double *mergearray;
  mergearray = (double *) malloc(I);
  local_i = temp_i = merge_i = I-1;

  while (merge_i>=0) {
    if (localdata[local_i]>tempdata[temp_i]) {
      mergearray[merge_i] = localdata[local_i];
      local_i--;
    }else{
      mergearray[merge_i] = tempdata[temp_i];
      temp_i--;
    }
    merge_i--;
  }
  for (int i = 0; i < I; i++) {
    localdata[i] = mergearray[i];
  }
  // memcpy(localdata, mergearray, I*sizeof(double));
  // free(mergearray);
  // free(tempdata);
}

int main(int argc, char **argv)
{
  int I, N, P, p, evenphase, evenprocess, tag = 10;
  double *localdata, *tempdata;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &P);
  MPI_Comm_rank(MPI_COMM_WORLD, &p);
  MPI_Status status;
  /* Find problem size N from command line */
  if (argc < 2) {
    printf("Invalid problem size < 2\n");
    exit(1);
  }
  N = atoi(argv[1]);
  /* local size. Modify if P does not divide N */
  if (N%P != 0){
    printf("P does not divide problem evenly\n");
    exit(1);
  }else{
    I = N/P;
  }
  localdata = (double *) malloc(I);
  tempdata = (double *) malloc(I);
  /* random number generator initialization */
  srandom(p+1);
  /* data generation */
  // printf("Rank: %d\n", p);
  for (int i = 0; i < I; i++){
    localdata[i] = ((double) random())/(RAND_MAX);
  }
  /*Inital sort of local lists*/
  qsort(localdata, I, sizeof(double), cmpfunc);
  // printf("Before sort\n");
  for (int i = 0; i < I; i++){
    printf("%f\n", localdata[i]);
  }
  /*Odd even phases*/
  for (int i = 0; i < P; i++) {
    if (p%2==0){
      // Even process
      if (i%2==0) {
        // Even phase
        MPI_Recv(tempdata, I, MPI_DOUBLE, p+1, tag, MPI_COMM_WORLD, &status);
        MPI_Send(localdata, I, MPI_DOUBLE, p+1, tag, MPI_COMM_WORLD);
        Merge_low(localdata, tempdata, I); // Merge smallest elements
      }else if(p>=2){
        // Odd phase
        MPI_Recv(tempdata, I, MPI_DOUBLE, p-1, tag, MPI_COMM_WORLD, &status);
        MPI_Send(localdata, I, MPI_DOUBLE, p-1, tag, MPI_COMM_WORLD);
        Merge_high(localdata, tempdata, I); // Merge largest elements
      }
    }
    else{
      // Odd process
      if (i%2==0) {
        // Even phase
        MPI_Send(localdata, I, MPI_DOUBLE, p-1, tag, MPI_COMM_WORLD);
        MPI_Recv(tempdata, I, MPI_DOUBLE, p-1, tag, MPI_COMM_WORLD, &status);
        Merge_high(localdata, tempdata, I); // Merge largest elements
      }else if(p<=P-3){
        // Odd phase
        MPI_Send(localdata, I, MPI_DOUBLE, p+1, tag, MPI_COMM_WORLD);
        MPI_Recv(tempdata, I, MPI_DOUBLE, p+1, tag, MPI_COMM_WORLD, &status);
        Merge_low(localdata, tempdata, I); // Merge smallest elements
      }
    }
  }
  int writeOK = 1;
  int writeTag = 10;
  FILE *fp0;
  if (p == 0) {
    writeOK = 0;
    fp0 = fopen("sorted.txt", "w"); // process 0 creates new file for output
  }else{
    MPI_Recv(&writeOK, 1, MPI_INT, p-1, writeTag, MPI_COMM_WORLD, &status);
    fp0 = fopen("sorted.txt", "a"); // Other processes append to same file
  }
  if (writeOK == 0){
    printf("Rank nr: %d writing\n", p);
    for (int j = 0; j < I; j++) {
      fprintf(fp0, "%f ", localdata[j]);
      fprintf(fp0, "\n");
    }
    fclose(fp0);
    if (p<P-1){
      MPI_Send(&writeOK, 1, MPI_INT, p+1, writeTag, MPI_COMM_WORLD);
    }
  }
  MPI_Finalize();
  exit(0);
}
