#include <unistd.h> // for sleep

#include "zynq.h"
#include "pulp_host.h"
#include "pulp_func.h"

#define CHECK_L2
#define PRINT_MAT

#ifndef SIZE
#define SIZE   16
//#define SIZE   32
#endif

int main() {
 
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
  pulp_reset(pulp,1);
  pulp_rab_free(pulp,0x0);
  pulp_init(pulp);
  
  pulp_stdout_clear(pulp,0);
  pulp_stdout_clear(pulp,1);
  pulp_stdout_clear(pulp,2);
  pulp_stdout_clear(pulp,3);

  /*
   * Body
   */
  // preparation
  __attribute__((aligned(64))) int mat_a[SIZE][SIZE];
  __attribute__((aligned(64))) int mat_b[SIZE][SIZE];
  __attribute__((aligned(64))) int mat_c[SIZE][SIZE];
  __attribute__((aligned(64))) int mat_d[SIZE][SIZE];  

  int i,j,k;

  // initialize the matrices including the result matrix (to allocate physical memory!)
  for(i=0; i<SIZE; i++) {
    for (j=0; j<SIZE; j++) {
      mat_a[i][j] = i*j+j;
      mat_b[i][j] = (SIZE-i)*(SIZE-j)+(SIZE-j);
      mat_c[i][j] = j;
    }
  }

#ifdef PRINT_MAT 
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
#endif

  // manually setup data structures (no compiler support right now)
  printf("Heterogenous OpenMP Matrix Multiplication Execution...\n");
 
  TaskDesc task_desc;
   
  char name[25];
  strcpy(name,"hsa_par_matrix_mul");
 
  task_desc.name = &name[0];
  task_desc.n_clusters = -1;
  task_desc.n_data = 3;
   
  DataDesc data_desc[task_desc.n_data];
  data_desc[0].ptr = (unsigned *)&mat_a;
  data_desc[0].size   = SIZE*SIZE*sizeof(int);
  data_desc[1].ptr = (unsigned *)&mat_b;
  data_desc[1].size   = SIZE*SIZE*sizeof(int);
  data_desc[2].ptr = (unsigned *)&mat_c;
  data_desc[2].size   = SIZE*SIZE*sizeof(int);
  
  task_desc.data_desc = data_desc;
 
  sleep(1);
 
  // issue the offload
  pulp_omp_offload_task(pulp,&task_desc);
   
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
      if (mat_c[i][j] != mat_d[i][j])
 	printf("ERROR: computation wrong!\n");
    }
  }
   
#ifdef PRINT_MAT
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
 
  //printf("mat_c[0][15] @ %#x :\n",(int)&mat_c[0][15]);
  //printf("mat_c[1][0] @ %#x :\n",(int)&mat_c[1][0]);
  //printf("mat_c[2][0] @ %#x :\n",(int)&mat_c[2][0]);
 
  printf("Matrix D @ %#x :\n",(int)&mat_d[0][0]);
  for (i=0;i<k;i++) {
    for (j=0;j<k;j++) {
      printf("%d\t",mat_d[i][j]);
    }
    printf("\n");
  }
#endif
 
#ifdef CHECK_L2 
  /*
   * Read matrices from L2
   */
  unsigned address, temp;
   
  //address = pulp_read32(pulp->mailbox.v_addr,MAILBOX_RDDATA_OFFSET_B,'b');
  pulp_mailbox_read(pulp,&address,1);
  printf("address =  %#x\n",address);
 
  address -= L2_MEM_BASE_ADDR;
 
  // print the matrix - for verification
  if ( SIZE > 16 )
    k = 16;
  else 
    k = SIZE;
 
  printf("Matrix A_L2 @ %#x :\n",address);
  for (i=0;i<k;i++) {
    temp = address + i*SIZE*4;
    for (j=0;j<k;j++) {
      printf("%d\t",pulp_read32(pulp->l2_mem.v_addr,temp,'b'));
      //printf("@ %#x\n",temp);
      temp += 4;
    }
    printf("\n");
  }
 
  //address = pulp_read32(pulp->mailbox.v_addr,MAILBOX_RDDATA_OFFSET_B,'b');
  pulp_mailbox_read(pulp,&address,1);  
  printf("address =  %#x\n",address);
 
  address -= L2_MEM_BASE_ADDR;
 
  // print the matrix - for verification
  if ( SIZE > 16 )
    k = 16;
  else 
    k = SIZE;
 
  printf("Matrix B_L2 @ %#x :\n",address);
  for (i=0;i<k;i++) {
    temp = address + i*SIZE*4;
    for (j=0;j<k;j++) {
      printf("%d\t",pulp_read32(pulp->l2_mem.v_addr,temp,'b'));
      //printf("@ %#x\n",temp);
      temp += 4;
    }
    printf("\n");
  }
 
  //address = pulp_read32(pulp->mailbox.v_addr,MAILBOX_RDDATA_OFFSET_B,'b');
  pulp_mailbox_read(pulp,&address,1);
  printf("address =  %#x\n",address);
 
  address -= L2_MEM_BASE_ADDR;
 
  // print the matrix - for verification
  if ( SIZE > 16 )
    k = 16;
  else 
    k = SIZE;
 
  printf("Matrix C_L2 @ %#x :\n",address);
  for (i=0;i<k;i++) {
    temp = address + i*SIZE*4;
    for (j=0;j<k;j++) {
      printf("%d\t",pulp_read32(pulp->l2_mem.v_addr,temp,'b'));
      //printf("@ %#x\n",temp);
      temp += 4;
    }
    printf("\n");
  }
#endif
 
  pulp_mailbox_read(pulp,&address,1);
  printf("address =  %#x\n",address);

  pulp_mailbox_read(pulp,&address,1);
  printf("address =  %#x\n",address);

  pulp_mailbox_read(pulp,&address,1);
  printf("address =  %#x\n",address);


  sleep(3);
  pulp_stdout_print(pulp,0);
  pulp_stdout_print(pulp,1);
  pulp_stdout_print(pulp,2);
  pulp_stdout_print(pulp,3);
  pulp_stdout_clear(pulp,0);
  pulp_stdout_clear(pulp,1);
  pulp_stdout_clear(pulp,2);
  pulp_stdout_clear(pulp,3);

  /*
   * Cleanup
   */
  pulp_rab_free(pulp,0);
  pulp_free_v_addr(pulp);
  sleep(1);
  pulp_munmap(pulp);
  
  return 0;
}
