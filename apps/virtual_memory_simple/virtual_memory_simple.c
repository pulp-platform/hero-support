#include <unistd.h> // for sleep

#include "../../lib/inc/zynq.h"
#include "../../lib/inc/pulp_host.h"
#include "../../lib/inc/pulp_func.h"


#ifndef SIZE
#define SIZE   16
#endif

int main(){
  
  /*
   * Initialization
   */ 
  // global variables
  PulpDev pulp_dev;
  PulpDev *pulp;
  pulp = &pulp_dev;

  // initialization of pulp
  pulp_reserve_v_addr(pulp);
  pulp_mmap(pulp);
  //sleep(1);
  //pulp_print_v_addr(pulp);
  //sleep(1);  
  pulp_reset(pulp);
  pulp_rab_free(pulp,0x0);
  pulp_init(pulp);
  
  //pulp_write32(pulp->gpio.v_addr,0x8,'b',0x80000000);
  
  /*
   * Body
   */
  // preparation
  int mat_a[SIZE][SIZE];
  int mat_b[SIZE][SIZE];
  int mat_c[SIZE][SIZE];
  int mat_d[SIZE][SIZE];

  //int ** mat_a = (int **)malloc(sizeof(int)*SIZE*SIZE);
  //int ** mat_b = (int **)malloc(sizeof(int)*SIZE*SIZE);
  //int ** mat_c = (int **)malloc(sizeof(int)*SIZE*SIZE);
  //int ** mat_d = (int **)malloc(sizeof(int)*SIZE*SIZE);


  int i,j,k;

  // initialize the matrices including the result matrix (to allocate physical memory!)
  for(i=0; i<SIZE; i++) {
    for (j=0; j<SIZE; j++) {
      mat_a[i][j] = i*j+j;
      mat_b[i][j] = (SIZE-i)*(SIZE-j)+(SIZE-j);
      mat_c[i][j] = j;
    }
  }

  // print the matrix - for verification
  if ( SIZE > 16 )
    k = 16;
  else 
    k = SIZE;
  
  printf("Matrix A @ %#x :\n",(int)&mat_a[0][0]);
  for (i=0;i<k;i++) {
    for (j=0;j<k;j++) {
      printf("%d\t",mat_a[i][j]);
    }
    printf("\n");
  }

  printf("Matrix B @ %#x :\n",(int)&mat_b[0][0]);
  for (i=0;i<k;i++) {
    for (j=0;j<k;j++) {
      printf("%d\t",mat_b[i][j]);
    }
    printf("\n");
  }

  // manually setup data structures (no compiler support right now)
  printf("Heterogenous OpenMP Matrix Multiplication Execution...\n");

  TaskDesc task_desc;
  
  char name[13];
  strcpy(name,"parMatrixMul");

  task_desc.name = &name[0];
  task_desc.n_clusters = -1;
  task_desc.n_data = 3;
  
  DataDesc data_desc[task_desc.n_data];
  data_desc[0].v_addr = (unsigned *)&mat_a;
  data_desc[0].size   = SIZE*SIZE*sizeof(int);
  data_desc[1].v_addr = (unsigned *)&mat_b;
  data_desc[1].size   = SIZE*SIZE*sizeof(int);
  data_desc[2].v_addr = (unsigned *)&mat_c;
  data_desc[2].size   = SIZE*SIZE*sizeof(int);
 
  task_desc.data_desc = data_desc;

  sleep(1);

  // issue the offload
  pulp_omp_offload_task(pulp,&task_desc);
  //pulp_omp_wait(pulp,ker_id);

  /*
   * Check
   */
  int checksum = 0;
  for(i=0; i<SIZE; i++) {
    for (k=0; k<SIZE; k++) {
      mat_d[i][k] = 0;
      for (j=0; j<SIZE; j++) {
	mat_d[i][k] += mat_a[i][j] * mat_b[j][k];
      }
      checksum += mat_d[i][k];
    }
  }
  printf("Checksum = %d\n",checksum);

  for(i=0; i<SIZE; i++) {
    for (j=0; j<SIZE; j++) {
      if (mat_c[i][j] != mat_c[i][j])
	printf("ERROR: computation wrong!\n");
     }
  }


  // print the matrix - for verification
  if ( SIZE > 16 )
    k = 16;
  else 
    k = SIZE;
  
  printf("Matrix C @ %#x :\n",(int)&mat_c[0][0]);
  for (i=0;i<k;i++) {
    for (j=0;j<k;j++) {
      printf("%d\t",mat_c[i][j]);
    }
    printf("\n");
  }

  printf("mat_c[0][15] @ %#x :\n",(int)&mat_c[0][15]);
  printf("mat_c[1][0] @ %#x :\n",(int)&mat_c[1][0]);
  printf("mat_c[2][0] @ %#x :\n",(int)&mat_c[2][0]);

  printf("Matrix D @ %#x :\n",(int)&mat_d[0][0]);
  for (i=0;i<k;i++) {
    for (j=0;j<k;j++) {
      printf("%d\t",mat_d[i][j]);
    }
    printf("\n");
  }

  sleep(2);

 /*
   * Cleanup
   */
  pulp_rab_free(pulp,0);
  pulp_free_v_addr(pulp);
  sleep(1);
  pulp_munmap(pulp);
  
  return 0;
}
