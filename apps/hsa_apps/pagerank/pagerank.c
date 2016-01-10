#include <unistd.h> // for sleep
#include <stdlib.h> // posix_memalign, abs
#include <time.h>   // for time measurements

#include "zynq.h"
#include "pulp_host.h"
#include "pulp_func.h"

#include "fixedptc.h"

#include "zynq_pmm_user.h"

#define PRINT_RESULT 0
#define PRINT_GRAPH 0
#define USE_FLOAT 0

#define ZYNQ_PMM 0

#define PAGE_SHIFT 12

#if ZYNQ_PMM == 1
// for cache miss rate measurement
int *zynq_pmm_fd;
int zynq_pmm;
#endif

typedef struct vertex vertex;

struct vertex{
  unsigned int vertex_id;
#if USE_FLOAT == 1
  float pagerank;
  float pagerank_next;
#else
  fixedptu pagerank;
  fixedptu pagerank_next;
#endif
  unsigned int n_successors;
  vertex ** successors;
};

float abs_float(float in)
{
  if (in >= 0)
    return in;
  else
    return -in;
}

int main(int argc, char *argv[]){
  
  int i, j, ret;
  unsigned status;
  
  printf("Testing pagerank...\n");      

  PulpDev pulp_dev;
  PulpDev *pulp;
  pulp = &pulp_dev;

  // reserve virtual addresses overlapping with PULP's internal physical address space
  pulp_reserve_v_addr(pulp);

  // setup time measurement
  struct timespec res, tp1, tp2, tp3, tp4, tp5, tp6, tp7, tp8;
  float start, end, duration;

  clock_getres(CLOCK_REALTIME,&res);

  /*
   * Preparation
   */
  char app_name[30];
  int timeout_s = 10;  
  int pulp_clk_freq_mhz = 50;
  int mem_sharing = 2;
  unsigned char use_acp = 0;

  strcpy(app_name,"pagerank");
  
  if (argc > 6) {
    printf("WARNING: More than 5 command line arguments are not supported. Those will be ignored.\n");
  }

  if (argc > 1)
    pulp_clk_freq_mhz = atoi(argv[1]);

  char file_name[30];
  
  if (argc > 2)
    strcpy(file_name,argv[2]);
  else
    strcpy(file_name,"test/tutte.txt");
  
  if (argc > 3)
    timeout_s = atoi(argv[3]);

  if (argc > 4)
    mem_sharing = atoi(argv[4]);
  if ((mem_sharing != 2) && (mem_sharing != 1))
    mem_sharing = 2;

  if (argc > 5)
    use_acp = atoi(argv[5]);

  /*************************************************************************/
  // build up the graph
   
  unsigned int n_vertices = 0;
  unsigned int n_edges = 0;
  unsigned int vertex_from = 0, vertex_to = 0;
  
  vertex * vertices;
    
  FILE * fp;
  if ((fp = fopen(file_name, "r")) == NULL) {
    printf("ERROR: Could not open input file.\n");
    return -ENOENT;
  }
  
  // parse input file to count the number of vertices
  // expected format: vertex_from vertex_to
  while (fscanf(fp, "%u %u", &vertex_from, &vertex_to) != EOF) {
    if (vertex_from > n_vertices)
      n_vertices = vertex_from;
    else if (vertex_to > n_vertices)
      n_vertices = vertex_to;
  }
  n_vertices++;
 
  // allocate memory for vertices
  vertices = (vertex *)malloc(n_vertices*sizeof(vertex));
  if (!vertices) {
    printf("Malloc failed for vertices.\n");
    return -ENOMEM;
  }
  memset((void *)vertices, 0, (size_t)(n_vertices*sizeof(vertex)));
  
  // parse input file to count the number of successors of each vertex
  fseek(fp, 0L, SEEK_SET);
  while (fscanf(fp, "%u %u", &vertex_from, &vertex_to) != EOF) {
    vertices[vertex_from].n_successors++;
    n_edges++;
  }

  // allocate memory for successor pointers
  for (i=0; i<n_vertices; i++) {
    vertices[i].vertex_id = i;
    if (vertices[i].n_successors > 0) {
      vertices[i].successors = (vertex **)malloc(vertices[i].n_successors*sizeof(vertex *));
      if (!vertices[i].successors) {
	printf("Malloc failed for successors of vertex %d.\n",i);
	return -ENOMEM;
      }
      memset((void *)vertices[i].successors, 0, (size_t)(vertices[i].n_successors*sizeof(vertex *)));
    }
    else
      vertices[i].successors = NULL;
    //vertices[i].successors = (vertex **)0xFFFFFFFF;
  }

  // parse input file to set up the successor pointers
  fseek(fp, 0L, SEEK_SET);
  while (fscanf(fp, "%d %d", &vertex_from, &vertex_to) != EOF) {
    for (i=0; i<vertices[vertex_from].n_successors; i++) {
      if (vertices[vertex_from].successors[i] == NULL) {
	vertices[vertex_from].successors[i] = &vertices[vertex_to];
	break;
      }
      else if (i==vertices[vertex_from].n_successors-1) {
	printf("Setting up the successor pointers of virtex %u failed",vertex_from);
	return -1;
      }
    }
  }
 
  fclose(fp);
  
  /*************************************************************************/
  // print the graph

  #if PRINT_GRAPH == 1
    for (i=0;i<n_vertices;i++) {
      if (vertices[i].n_successors > 0) {
	for (j=0;j<vertices[i].n_successors;j++) {
	  printf("Vertex %u @ %p:\t Successor %u @ %p = Vertex %u\n",
		 i, &(vertices[i]), j, vertices[i].successors[j],
		 vertices[i].successors[j]->vertex_id);
	}
	printf("Successors @ %p\n",vertices[i].successors);
      }  
      else
	printf("Vertex %u @ %p\n",i, &(vertices[i]));
    }
  #endif

  /*************************************************************************/
  // print information

  printf("----------------------------------------\n");
  printf("        Pagerank Information:\n");
  printf("----------------------------------------\n");

  printf("# vertices = %d, # edges = %d\n",n_vertices, n_edges);
  printf("# edges per vertex = %.2f\nbreak-even point = %.2f\n",
	 (float)n_edges/n_vertices, (float)n_vertices/sizeof(vertex *));
  printf("graph density = %.6f\n",(float)n_edges/(n_vertices*(n_vertices-1)));

  unsigned int size_b_vertices;
  size_b_vertices = (unsigned int)(n_vertices*sizeof(vertex));

  printf("graph size = %d Bytes = %#x\n",size_b_vertices+n_edges*sizeof(vertex *), size_b_vertices+n_edges*sizeof(vertex *));

  unsigned int n_pages_vertices;
  unsigned int n_pages_successors;

  unsigned int vertices_start, vertices_end;

  vertices_start = (unsigned int)vertices & BF_MASK_GEN(PAGE_SHIFT,32-PAGE_SHIFT);
  vertices_end = ((unsigned int)vertices + size_b_vertices) & BF_MASK_GEN(PAGE_SHIFT,32-PAGE_SHIFT);

  n_pages_vertices = (BF_GET((unsigned int)vertices,0,PAGE_SHIFT) + size_b_vertices) >> PAGE_SHIFT;
  if (BF_GET(BF_GET((unsigned int)vertices,0,PAGE_SHIFT) + size_b_vertices,0,PAGE_SHIFT)) n_pages_vertices++; // remainder
  //printf("%d\n",n_pages_vertices);

  unsigned int * addr_start, * addr_end;
  unsigned int temp2;
  unsigned int start_ok, end_ok;
 
  addr_start = (unsigned int *)malloc(n_vertices*sizeof(unsigned int*));
  addr_end   = (unsigned int *)malloc(n_vertices*sizeof(unsigned int*));

  n_pages_successors = 0;
  for (i=0;i<n_vertices;i++) {
    if (vertices[i].n_successors > 0) {

      temp2 = (unsigned int)vertices[i].successors;
      addr_start[i] = (temp2 & BF_MASK_GEN(PAGE_SHIFT,32-PAGE_SHIFT));
      addr_end[i]   = ((temp2 + vertices[i].n_successors*sizeof(vertex *)) & BF_MASK_GEN(PAGE_SHIFT,32-PAGE_SHIFT));
      //printf("addr_start - addr_end: %#x = %#x\n",addr_start[i],addr_end[i]);

      start_ok = 0;
      end_ok = 0;
      if (addr_start[i] == vertices_start)
	start_ok = 1;
      if (addr_end[i] ==  vertices_end)
	end_ok = 1;;
      for (j=0; j<i; j++) {
	if (addr_start[i] == addr_start[j]) 
	  start_ok = 1;
	if (addr_end[i] == addr_end[j])
	  end_ok = 1;
      }
    
      if ((start_ok == 0) || (end_ok == 0)) {
	n_pages_successors++;
	if (addr_start[i] != addr_end[i])
	  n_pages_successors++;
      }
    }
  }
    
  free(addr_start);
  free(addr_end);

  printf("# virtual 4KB pages touched = %d \n",n_pages_vertices+n_pages_successors);
  printf("----------------------------------------\n");

  /*************************************************************************/
#if ZYNQ_PMM == 1
  // setup cache miss rate measurement
  char proc_text[ZYNQ_PMM_PROC_N_CHARS_MAX];
  long long counter_values[(N_ARM_CORES+1)*2];
  double cache_miss_rates[(N_ARM_CORES+1)*2];
  zynq_pmm_fd = &zynq_pmm;
  ret = zynq_pmm_open(zynq_pmm_fd);
  if (ret)
    return ret;

  // delete the accumulation counters
  ret = zynq_pmm_parse(proc_text, counter_values, -1);

  // reset cache counters
  zynq_pmm_read(zynq_pmm_fd,proc_text);
#endif

  /*************************************************************************/
  // compute the pagerank

  unsigned int n_iterations = 25;
  float alpha = 0.85;
  float eps   = 0.000001;
  
  // run on the host
  unsigned int i_iteration;

  clock_gettime(CLOCK_REALTIME,&tp7);

#if USE_FLOAT == 1
   float value, diff;
   float pr_dangling_factor = alpha / (float)n_vertices;   // pagerank to redistribute from dangling nodes  
   float pr_dangling;
   float pr_random_factor = (1-alpha) / (float)n_vertices; // random portion of the pagerank 
   float pr_random;
   float pr_sum, pr_sum_inv, pr_sum_dangling;
   float temp;
   
   // initialization
   for (i=0;i<n_vertices;i++) {
     vertices[i].pagerank = 1 / (float)n_vertices;
     vertices[i].pagerank_next =  0;
   }
   
   pr_sum = 0;
   pr_sum_dangling = 0;
   for (i=0; i<n_vertices; i++) {
     pr_sum += vertices[i].pagerank;
     if (!vertices[i].n_successors)
       pr_sum_dangling += vertices[i].pagerank;
   }  
   
   i_iteration = 0;
   diff = eps+1;
   
   while ( (diff > eps) && (i_iteration < n_iterations) ) {
     
     //printf("pr_dangling = %.6f, pr_random = %.6f \n",pr_dangling, pr_random);
   
     for (i=0;i<n_vertices;i++) {
       if (vertices[i].n_successors) 
   	value = (alpha/vertices[i].n_successors)*vertices[i].pagerank;
       else
   	value = 0;
       //printf("vertex %d: value = %.6f \n",i,value);
       for (j=0;j<vertices[i].n_successors;j++) {
   	vertices[i].successors[j]->pagerank_next += value;
       }
     }   
   
     // for normalization
     pr_sum_inv = 1/pr_sum;
     
     // alpha
     pr_dangling = pr_dangling_factor * pr_sum_dangling;
     pr_random = pr_random_factor * pr_sum;
   
     pr_sum = 0;
     pr_sum_dangling = 0;
     
     diff = 0;
     for (i=0;i<n_vertices;i++) {
       
       // update pagerank
       temp = vertices[i].pagerank;
       vertices[i].pagerank = vertices[i].pagerank_next*pr_sum_inv + pr_dangling + pr_random;
       vertices[i].pagerank_next = 0;
   
       // for normalization in next cycle
       pr_sum += vertices[i].pagerank;
       if (!vertices[i].n_successors)
   	pr_sum_dangling += vertices[i].pagerank;
   
       // convergence
       diff += abs_float(temp - vertices[i].pagerank);
     }
     printf("Iteration %u:\t diff = %.12f\n", i_iteration, diff);
   
     i_iteration++;
   }
   
   /*************************************************************************/
   // print
   
#if PRINT_RESULT > 1
   for (i=0;i<n_vertices;i++) {
     printf("Vertex %u:\tpagerank = %.6f\n", i, vertices[i].pagerank);
   }
#endif

#else // USE_FLOAT != 1

  /*************************************************************************/
  // compute the pagerank in fixedpt format
  
  // shared variables
  //unsigned int n_iterations = 25;
  fixedptu eps_f   = fixedpt_rconst_HP(eps);
  fixedptu alpha_f = fixedpt_rconst_HP(alpha);
   
  fixedptu diff_f = fixedpt_rconst_HP(1) + eps_f;
  //fixedptu n_vertices_f = fixedpt_fromint(n_vertices);
  fixedptu n_vertices_f = fixedpt_frominttoHP(n_vertices);
  
  // pagerank to redistribute from dangling nodes  
  //fixedptu pr_dangling_factor_f =    
  //  fixedpt_to_HP(fixedpt_div(fixedpt_from_HP(alpha_f),n_vertices_f));
  fixedptu pr_dangling_factor_f =
    fixedpt_div_HP(alpha_f,n_vertices_f);
  fixedptu pr_dangling_f;

  // random portion of the pagerank 
  //fixedptu pr_random_factor_f = 
  //  fixedpt_to_HP(fixedpt_div(fixedpt_from_HP(fixedpt_rconst_HP(1)-alpha_f),n_vertices_f));
  fixedptu pr_random_factor_f = 
    fixedpt_div_HP((fixedpt_rconst_HP(1)-alpha_f),n_vertices_f);
  fixedptu pr_random_f;
    
  fixedptu pr_sum_f = 0, pr_sum_inv_f;
  fixedptu pr_sum_dangling_f = 0;

  // initialization
  for (i=0; i<n_vertices; i++) {
    //vertices[i].pagerank = fixedpt_to_HP(fixedpt_div(fixedpt_rconst(1),n_vertices_f));
    vertices[i].pagerank = fixedpt_div_HP(fixedpt_rconst_HP(1),n_vertices_f);
    vertices[i].pagerank_next = fixedpt_frominttoHP(0);

    pr_sum_f += (vertices+i)->pagerank;

    if ( (vertices+i)->n_successors == 0 )
      pr_sum_dangling_f += (vertices+i)->pagerank;
  }

  i_iteration = 0;
  diff_f = eps_f+1; 

  fixedptu value_f; 
  fixedptu temp_f; 

  unsigned int n_successors;
  fixedptu n_successors_inv_f;

  // pagerank
  while ( (diff_f > eps_f) && (i_iteration < n_iterations) ) {
    for (i=0;i<n_vertices;i++) {
      n_successors = (vertices+i)->n_successors;

      if (n_successors) {
	n_successors_inv_f = fixedpt_div_HP(fixedpt_frominttoHP(1),fixedpt_frominttoHP(n_successors));
	value_f = fixedpt_mul_HP(fixedpt_mul_HP(alpha_f,n_successors_inv_f),(vertices+i)->pagerank);
      }
      else
	value_f = fixedpt_frominttoHP(0);
 
      if (n_successors) {
	for (j=0;j<n_successors;j++) {
	  //printf("Successor ptr %d @ %#x = %#x \n",j,
	  //	 (unsigned)&((vertices+i)->successors[j]),
	  //	 (unsigned)((vertices+i)->successors[j]));
	  (vertices+i)->successors[j]->pagerank_next += value_f;
	}
      }
    }

    // for normalization
    pr_sum_inv_f = fixedpt_div_HP(fixedpt_frominttoHP(1),pr_sum_f);

    // alpha
    pr_dangling_f = fixedpt_mul_HP(pr_dangling_factor_f,pr_sum_dangling_f);
    pr_random_f   = fixedpt_mul_HP(pr_random_factor_f,pr_sum_f);

    pr_sum_f = fixedpt_frominttoHP(0);
    pr_sum_dangling_f = fixedpt_frominttoHP(0);
 	  
    diff_f = fixedpt_frominttoHP(0);

    // update pagerank
    for (i=0; i<n_vertices; i++) {
 	
      temp_f = (vertices+i)->pagerank;
      (vertices+i)->pagerank = fixedpt_mul_HP((vertices+i)->pagerank_next,pr_sum_inv_f) + pr_dangling_f + pr_random_f;
    
      (vertices+i)->pagerank_next = fixedpt_frominttoHP(0);

      // for normalization in next cycle
      pr_sum_f += (vertices+i)->pagerank;
      if ((vertices+i)->n_successors == 0)
	pr_sum_dangling_f += (vertices+i)->pagerank;
 	
      // convergence
      diff_f += fixedpt_to_HP(fixedpt_abs(fixedpt_from_HP(temp_f - (vertices+i)->pagerank)));
    }

    printf("Iteration %u:\t diff = ",i_iteration);
    fixedpt_print_HP(diff_f);

    i_iteration++;
  }
  printf("Number of iterations executed = %d\n",i_iteration);

  /*************************************************************************/
  // print cache miss rate
#if ZYNQ_PMM == 1 
    zynq_pmm_read(zynq_pmm_fd, proc_text);
    zynq_pmm_parse(proc_text, counter_values, 1); // accumulate cache counter values

    zynq_pmm_compute_rates(cache_miss_rates, counter_values); // compute cache miss rates
    double miss_rate_0 = cache_miss_rates[0]*100;
    double miss_rate_1 = cache_miss_rates[1]*100;
    printf("\n###########################################################################\n");
    printf("Host Kernel L1 D-Cache Miss Rates: %.2f %% (Core 0), %.2f %% (Core 1)\n",miss_rate_0,miss_rate_1);
    printf("\n###########################################################################\n");

     zynq_pmm_close(zynq_pmm_fd);
#endif

  /*************************************************************************/
  // print
  
  #if PRINT_RESULT > 0
  for (i=0;i<n_vertices;i++) {
    printf("Vertex %u:\tpagerank = ", i);
    fixedpt_print_HP(vertices[i].pagerank);
  }
  #endif

  /*************************************************************************/
  // store ARM results for later comparison with PULP results
  fixedptu * arm_pageranks = malloc(n_vertices*sizeof(fixedptu));
  if (!arm_pageranks) {
    printf("Malloc failed for arm_pageranks.\n");
    return -ENOMEM;
  }
  for (i=0;i<n_vertices;i++) {
    arm_pageranks[i] = vertices[i].pagerank;
    vertices[i].pagerank = 0xFFFFFFFF;
  }

#endif

  clock_gettime(CLOCK_REALTIME,&tp8);

  /*************************************************************************/

  /*
   * Initialization
   */
  printf("PULP Initialization\n");
 
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

  // measure the actual clock frequency
  pulp_clk_freq_mhz = pulp_clking_measure_freq(pulp);
  printf("PULP actually running @ %d MHz.\n",pulp_clk_freq_mhz);
  
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
  pulp_rab_mh_enable(pulp, use_acp);
  
  clock_gettime(CLOCK_REALTIME,&tp1);

  // write PULP_START
  pulp_write32(pulp->mailbox.v_addr, MAILBOX_WRDATA_OFFSET_B, 'b', PULP_START);

  unsigned int * n_vertices_cv;
  unsigned n_vertices_cp;
  vertex * vertices_cv;
  unsigned vertices_cp;
  vertex *** successor_ptrs_cv;
  
  if (mem_sharing == 1) {   
    // copy and parse the data structure to contiguous L3
    n_vertices_cv = (unsigned int *)pulp_l3_malloc(pulp, sizeof(unsigned), &n_vertices_cp);
    if (!n_vertices_cv) {
      printf("Malloc failed for n_vertices_cv.\n");
      return -ENOMEM;
    }
    *n_vertices_cv = n_vertices;

    // allocate memory for vertices
    vertices_cv = (vertex *)pulp_l3_malloc(pulp, n_vertices*sizeof(vertex), (unsigned *)&vertices_cp);
    if (!vertices_cv) {
      printf("Malloc failed for vertices_cv.\n");
      return -ENOMEM;
    }
    memset((void *)vertices_cv, 0, (size_t)(n_vertices*sizeof(vertex)));

    // allocate memory for virtual successor pointers
    successor_ptrs_cv = (vertex ***)malloc(n_vertices*sizeof(vertex **));
    if (!successor_ptrs_cv) {
      printf("Malloc failed for successor_ptrs_cv.\n");
      return -ENOMEM;
    }
    memset((void *)successor_ptrs_cv, 0, (size_t)(n_vertices*sizeof(vertex *)));

    // parse and copy the graph
    for (i=0; i<n_vertices; i++) {
      vertices_cv[i].vertex_id = vertices[i].vertex_id;
      vertices_cv[i].pagerank = vertices[i].pagerank;
      vertices_cv[i].pagerank_next = vertices[i].pagerank_next;
      vertices_cv[i].n_successors = vertices[i].n_successors;

      // allocate memory for physical successor pointers
      if (vertices[i].n_successors > 0) {
	successor_ptrs_cv[i] = (vertex **)pulp_l3_malloc(pulp, vertices[i].n_successors*sizeof(vertex *),
							  (unsigned *)&(vertices_cv[i].successors));
	if (!successor_ptrs_cv) {
	  printf("Malloc failed for successor_ptrs_cv[%d].\n",i);
	  return -ENOMEM;
	}
	memset((void *)successor_ptrs_cv[i], 0, (size_t)(vertices[i].n_successors*sizeof(vertex *)));
      }       
      else
	vertices_cv[i].successors = NULL;
    }
    
    // set up successor pointers
    vertex ** v_ptr;
    for (i=0; i<n_vertices; i++) {
      if (vertices[i].n_successors > 0)  {
	v_ptr = successor_ptrs_cv[i];
	for (j=0; j<vertices[i].n_successors; j++) {
	  // write physical address of successors into the graph in contiguous L3 (use virtual pointer)
	  v_ptr[j] = (vertex *)((unsigned)vertices_cp + (unsigned)(vertices[i].successors[j]->vertex_id*sizeof(vertex)));
	}	
      }	
    }

    // pass data to mailbox
    status = 1;
    while (status) 
      status = pulp_read32(pulp->mailbox.v_addr, MAILBOX_STATUS_OFFSET_B, 'b') & 0x2;	
    pulp_write32(pulp->mailbox.v_addr, MAILBOX_WRDATA_OFFSET_B, 'b', (unsigned)*n_vertices_cv);
    printf("Wrote number of vertices %d to mailbox.\n", (unsigned)*n_vertices_cv);
    while (status) 
      status = pulp_read32(pulp->mailbox.v_addr, MAILBOX_STATUS_OFFSET_B, 'b') & 0x2;	
    pulp_write32(pulp->mailbox.v_addr, MAILBOX_WRDATA_OFFSET_B, 'b', (unsigned)vertices_cp);
    printf("Wrote address %#x of vertices_cp to mailbox.\n", (unsigned)vertices_cp);
  }
  else { // mem_sharing == 2 
    // pass data to mailbox
    status = 1;
    while (status) 
      status = pulp_read32(pulp->mailbox.v_addr, MAILBOX_STATUS_OFFSET_B, 'b') & 0x2;	
    pulp_write32(pulp->mailbox.v_addr, MAILBOX_WRDATA_OFFSET_B, 'b', (unsigned)n_vertices);
    printf("Wrote number of vertices %d to mailbox.\n", (unsigned)n_vertices);
    while (status) 
      status = pulp_read32(pulp->mailbox.v_addr, MAILBOX_STATUS_OFFSET_B, 'b') & 0x2;	
    pulp_write32(pulp->mailbox.v_addr, MAILBOX_WRDATA_OFFSET_B, 'b', (unsigned)vertices);
    printf("Wrote address %#x of vertices to mailbox.\n", (unsigned)vertices);
  }
   
  clock_gettime(CLOCK_REALTIME,&tp2);

  /*************************************************************************/

  clock_gettime(CLOCK_REALTIME,&tp3);

  // start execution
  pulp_exe_start(pulp);
  
  // wait for initialization end
  pulp_mailbox_read(pulp, (unsigned *)&ret, 1);
  if (ret != PULP_READY)
    printf("ERROR: Initialization failed\n");

  // free RAB
  pulp_rab_free(pulp,0);
  
  // continue
  pulp_write32(pulp->mailbox.v_addr,MAILBOX_WRDATA_OFFSET_B,'b',PULP_START);
    
  clock_gettime(CLOCK_REALTIME,&tp3);

  // wait for end of computation
  pulp_exe_wait(pulp,timeout_s);

  clock_gettime(CLOCK_REALTIME,&tp4);

  /*************************************************************************/

  // stop execution
  pulp_exe_stop(pulp);

  clock_gettime(CLOCK_REALTIME,&tp5);

  if (mem_sharing == 1) {
    // copy back results and free data
    for (i=0; i<n_vertices; i++) {
      vertices[i].pagerank = vertices_cv[i].pagerank;
      pulp_l3_free(pulp, (unsigned)successor_ptrs_cv[i], (unsigned)vertices_cv[i].successors);
    }
    free(successor_ptrs_cv);
    pulp_l3_free(pulp, (unsigned)vertices_cv, (unsigned)vertices_cp);
    pulp_l3_free(pulp, (unsigned)n_vertices_cv, n_vertices_cp);
  }

  clock_gettime(CLOCK_REALTIME,&tp6);

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

  /*************************************************************************/

  // compute times
  start = ((float)tp7.tv_sec)*1000000000 + (float)tp7.tv_nsec; 
  end = ((float)tp8.tv_sec)*1000000000 + (float)tp8.tv_nsec; 
  duration = (end - start)/1000000000;
  printf("Host execution time = %.6f s \n",duration);

  start = ((float)tp1.tv_sec)*1000000000 + (float)tp1.tv_nsec; 
  end = ((float)tp2.tv_sec)*1000000000 + (float)tp2.tv_nsec; 
  duration = (end - start)/1000000000;
  printf("Offload out time    = %.6f s \n",duration);

  start = ((float)tp3.tv_sec)*1000000000 + (float)tp3.tv_nsec; 
  end = ((float)tp4.tv_sec)*1000000000 + (float)tp4.tv_nsec; 
  duration = (end - start)/1000000000;
  printf("Execution time      = %.6f s \n",duration);

  start = ((float)tp5.tv_sec)*1000000000 + (float)tp5.tv_nsec; 
  end = ((float)tp6.tv_sec)*1000000000 + (float)tp6.tv_nsec; 
  duration = (end - start)/1000000000;
  printf("Offload in time     = %.6f s \n",duration);
  
  /*************************************************************************/

#if USE_FLOAT == 0
  /*
   * Compare results
   */
  /*************************************************************************/
  unsigned n_false = 0;  
  printf("Comparing final results of ARM and PULP.\n");
  for (i=0;i<n_vertices;i++) {
    if (arm_pageranks[i] != vertices[i].pagerank) {
      if (n_false < 3) {
	printf("Different pageranks for Vertex %d: \n",i);
	printf("Pagerank ARM = ");
	fixedpt_print_HP(arm_pageranks[i]);
	printf("Pagerank PULP = ");
	fixedpt_print_HP((vertices+i)->pagerank);
      }
      n_false++;
    }
  }
  printf("Done.\n");
  printf("Comparison failed for %d vertices.\n",n_false);
#endif

  /*
   * Cleanup
   */
  /*************************************************************************/
#if USE_FLOAT == 0
  free(arm_pageranks);
#endif
  for (i=0;i<n_vertices;i++) {
    free(vertices[i].successors);
  } 
  free(vertices);
  
  /*************************************************************************/  
  printf("PULP Cleanup\n");
  pulp_rab_free(pulp,0);
  pulp_free_v_addr(pulp);
  sleep(1);
  pulp_munmap(pulp);
  
  return 0;
}
