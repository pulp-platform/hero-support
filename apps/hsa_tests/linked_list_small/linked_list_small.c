#include <unistd.h> // for sleep
#include <stdlib.h> // posix_memalign
#include <time.h>   // for time measurements

#include "zynq.h"
#include "pulp_host.h"
#include "pulp_func.h"

// printing
#define _printhexp(x,y)	    {printf(strcat(x, " %x\n"),y);}
#define _printdecp(x,y)	    {printf(strcat(x, " %d\n"),y);}

/* Size of the chunk of memory between two nodes of the list (in words) */
#define SPACING_SIZE_B          0x1000
#define	SIZEOF_CONTIGUOUS_CHUNK	SPACING_SIZE_B/4

typedef struct
{
  unsigned int *next;
  unsigned int val;
} node;

unsigned int mylist[5*SIZEOF_CONTIGUOUS_CHUNK];

#define address_of_list_element(x)	&mylist[(x*SIZEOF_CONTIGUOUS_CHUNK)]

int main(int argc, char *argv[]){
  
  int ret;
  unsigned status;
  
  printf("Testing linked_list_small...\n");      

  /*
   * Preparation
   */
  char app_name[30];
  int timeout_s = 10;  
  int pulp_clk_freq_mhz = 50;

  strcpy(app_name,"linked_list_small");
  
  if (argc > 2) {
    printf("WARNING: More than 1 command line argument is not supported. Those will be ignored.\n");
  }

  if (argc > 1)
    pulp_clk_freq_mhz = atoi(argv[1]);

  /*************************************************************************/
  // linked list
  node *head = (node *) address_of_list_element(1);
  int n_elements = 5;

  printf("mylist @ %p\n",mylist);
  printf("&mylist[0] = %#x\n",(unsigned int)&mylist[0]);
  printf("head = %#x\n",(unsigned int)head);

  printf("address_of_list_element(0) = %#x\n",(unsigned int)address_of_list_element(0));
  printf("address_of_list_element(1) = %#x\n",(unsigned int)address_of_list_element(1));
  printf("address_of_list_element(2) = %#x\n",(unsigned int)address_of_list_element(2));
  printf("address_of_list_element(3) = %#x\n",(unsigned int)address_of_list_element(3));
  printf("address_of_list_element(4) = %#x\n",(unsigned int)address_of_list_element(4));

  /* Initialize list */
  head->next = (unsigned int *) address_of_list_element(3);
  head->val = 1;
  printf("node %d: head @ %#x, val = %#x, next = %#x\n", 0,
	 (unsigned int)head, head->val, (unsigned int)head->next);
  head = (node *)head->next;

  head->next = (unsigned int *) address_of_list_element(0);
  head->val = 2;
  printf("node %d: head @ %#x, val = %#x, next = %#x\n", 1, 
	 (unsigned int)head, head->val, (unsigned int)head->next);
  head = (node *)head->next;
  
  head->next = (unsigned int *) address_of_list_element(2);
  head->val = 3;
  printf("node %d: head @ %#x, val = %#x, next = %#x\n", 2, 
	 (unsigned int)head, head->val, (unsigned int)head->next);
  head = (node *)head->next;
  
  head->next = (unsigned int *) address_of_list_element(4);
  head->val = 4;
  printf("node %d: head @ %#x, val = %#x, next = %#x\n", 3,
	 (unsigned int)head, head->val, (unsigned int)head->next);
  head = (node *)head->next;
  
  head->next = (unsigned int *) address_of_list_element(1);
  head->val = 5;
  printf("node %d: head @ %#x, val = %#x, next = %#x\n", 4, 
	 (unsigned int)head, head->val, (unsigned int)head->next);
  head = (node *) address_of_list_element(1);
  
  /*************************************************************************/

  /*
   * Initialization
   */
  printf("PULP Initialization\n");
 
  PulpDev pulp_dev;
  PulpDev *pulp;
  pulp = &pulp_dev;

  // reserve virtual addresses overlapping with PULP's internal physical address space
  pulp_reserve_v_addr(pulp);

  pulp_mmap(pulp);
  //pulp_print_v_addr(pulp);
  pulp_reset(pulp,1);
  
  // set desired clock frequency
  if (pulp_clk_freq_mhz != 50) {
    ret = pulp_clking_set_freq(pulp,pulp_clk_freq_mhz);
    if (ret > 0) {
      printf("PULP Running @ %d MHz.\n",ret);
      pulp_clk_freq_mhz = ret;
    }
    else
      printf("ERROR: setting clock frequency failed");
  }

  pulp_rab_free(pulp,0x0);

  // initialization of PULP, static RAB rules (mailbox, L2, ...)
  pulp_init(pulp);

  // clear memories?
  
  // clear stdout
  pulp_stdout_clear(pulp,0);
  pulp_stdout_clear(pulp,1);
  pulp_stdout_clear(pulp,2);
  pulp_stdout_clear(pulp,3);

  /*
   * Body
   */
  printf("PULP Execution\n");
  // load binary
  pulp_load_bin(pulp,app_name);

  /*************************************************************************/
 
  // enable RAB miss handling
  pulp_rab_mh_enable(pulp);
  
  // write PULP_START
  pulp_write32(pulp->mailbox.v_addr, MAILBOX_WRDATA_OFFSET_B, 'b', PULP_START);

  // pass pointers to mailbox
  status = 1;
  while (status) 
    status = pulp_read32(pulp->mailbox.v_addr, MAILBOX_STATUS_OFFSET_B, 'b') & 0x2;	
  pulp_write32(pulp->mailbox.v_addr, MAILBOX_WRDATA_OFFSET_B, 'b', (unsigned)head);
  printf("Wrote address %#x of head to mailbox.\n", (unsigned)head);
   
  /*************************************************************************/

  // start execution
  pulp_exe_start(pulp);
  
  usleep(2000000);

  // wait for end of computation
  pulp_exe_wait(pulp,timeout_s);

  // stop execution
  pulp_exe_stop(pulp);

  /*************************************************************************/

  // disable RAB miss handling
  pulp_rab_mh_disable(pulp);

  /*************************************************************************/

  // -> poll stdout
  pulp_stdout_print(pulp,0);
  pulp_stdout_print(pulp,1);
  pulp_stdout_print(pulp,2);
  pulp_stdout_print(pulp,3);

  // clear stdout
  pulp_stdout_clear(pulp,0);
  pulp_stdout_clear(pulp,1);
  pulp_stdout_clear(pulp,2);
  pulp_stdout_clear(pulp,3);

  /*
   * Cleanup
   */
  /*************************************************************************/
  
  
  /*************************************************************************/  

  printf("PULP Cleanup\n");
  pulp_rab_free(pulp,0);
  pulp_free_v_addr(pulp);
  sleep(1);
  pulp_munmap(pulp);
  
  return 0;
}
