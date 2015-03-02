#include "ompOffload.h"
#include "omp-common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ld/p2012_lnx_dd_host.h"
#include "pthread.h"

int receivedMsg = 0;
int cnt = 0;
// char *rcvMsg = NULL;
pthread_t rcvThread; //FIXME close it at the end!!!
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

omp_termination_t *omp_terminated = NULL;

void ompMsgCallback(unsigned int msg) {    
//   printf("[ompMsgCallback]try %d\n", cnt);
  pthread_mutex_lock(&mutex);
//   printf("[ompMsgCallback]ok %d\n", cnt++);
  omp_termination_t *current_termination = (omp_termination_t *)malloc(sizeof(omp_termination_t));
  if(omp_terminated == NULL)
  {
    current_termination->next = NULL;
    current_termination->prev = NULL;
    omp_terminated = current_termination;
  }
  else
  {
    current_termination->prev = NULL;
    current_termination->next = omp_terminated;
    omp_terminated->prev = current_termination;
    omp_terminated = current_termination;
  }
  
  current_termination->value = ((uint32_t) msg >> 8);
//   printf("[ompMsgCallback]\t%p\t%d\n", current_termination->value,cnt++);
  
  receivedMsg++;
  pthread_cond_broadcast(&cond);
  pthread_mutex_unlock(&mutex);
}

int sthorm_omp_rt_init()
{
    if (connectToDevice()) return -1; 

    registerIncomingMsgCallback(FABRIC_RT_OMP_ID, ompMsgCallback);

    return 0;
}

int GOMP_offload_wait(omp_offload_t *omp_offload, uint32_t fomp_offload)
{
    gomp_offload_wait(omp_offload, fomp_offload);
    gomp_offload_datacpy_in(omp_offload, fomp_offload);
    
    return 0;
}

inline int gomp_offload_datacpy_in(omp_offload_t *omp_offload, uint32_t fomp_offload) {
    
    if(omp_offload->kernelInfo.n_data)
    {
        int i;
        for (i = 0; i < omp_offload->kernelInfo.n_data; i++)
        {           
            switch(omp_offload->kernelInfo.data[i].type)
            {
                case 0:
                    //INOUT                    
                    memcpy((void *) omp_offload->kernelInfo.data[i].ptr, (void *) omp_offload->kernelInfo.hdata_ptrs[i] , omp_offload->kernelInfo.data[i].size);
                    p2012_socmem_free((uint32_t)omp_offload->kernelInfo.hdata_ptrs[i], omp_offload->kernelInfo.host_args_ptr[i]);
                    break;
                    
                case 1:
                    //IN                    
                    p2012_socmem_free((uint32_t)omp_offload->kernelInfo.hdata_ptrs[i], omp_offload->kernelInfo.host_args_ptr[i]);
                    break;
                    
                case 2:
                    //OUT                    
                    memcpy((void *) omp_offload->kernelInfo.data[i].ptr, (void *) omp_offload->kernelInfo.hdata_ptrs[i] , omp_offload->kernelInfo.data[i].size);
                    p2012_socmem_free((uint32_t)omp_offload->kernelInfo.hdata_ptrs[i], omp_offload->kernelInfo.host_args_ptr[i]);
                    break;
                    
                default:
                    //NONE
                    break;
            }
        }
        free(omp_offload->kernelInfo.hdata_ptrs);
        p2012_socmem_free((uint32_t)omp_offload->kernelInfo.host_args_ptr, (uint32_t) omp_offload->kernelInfo.args);
    }
    
    p2012_socmem_free((uint32_t)omp_offload, fomp_offload);
   return 0;
}

inline int gomp_offload_wait(omp_offload_t *omp_offload, uint32_t fomp_offload){
    int exit = 0;
    
    pthread_mutex_lock(&mutex);
    while(1) {
        omp_termination_t *curr = omp_terminated;
        
        while(curr!=NULL)
        {
            //printf("[GOMP_offload_wait]\t%p == %p\n", curr->value, ((unsigned int) omp_offload>>8));
            
            if(curr->value == ((unsigned int) omp_offload>>8))
            {             
                receivedMsg--;
                if(curr->prev != NULL)
                    curr->prev->next = curr->next;
                
                if(curr->next != NULL)
                    curr->next->prev = curr->prev;
                
                if (curr == omp_terminated) {
                    if((curr->next == NULL) && (curr->prev == NULL)) //i am alone
                        omp_terminated = NULL;
                    else
                        omp_terminated = curr->next;
                }
                
                free(curr);
                exit = 1;
                break;
            }
            else
                curr = curr->next;
        }
        if(exit)
            break;
        else
            pthread_cond_wait(&cond, &mutex);
    }
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);
    //printf("[GOMP_offload_wait] wait....done\n");
    
    return 0;
}

int GOMP_offload_nowait(ompOffload_desc_t *desc, omp_offload_t **omp_offload, uint32_t *fomp_offload){
    gomp_offload_datacpy_out(desc, omp_offload, fomp_offload);    
    gomp_offload_enqueue(fomp_offload);
    return 0;
}

inline int gomp_offload_datacpy_out(ompOffload_desc_t *desc, omp_offload_t **omp_offload, uint32_t *fomp_offload) {
    int i;
    
    omp_offload_t *new_offload = (omp_offload_t *)p2012_socmem_alloc(sizeof(omp_offload_t), fomp_offload);
    new_offload->no_sync.header.type = FABRIC_RT_OMP_HTOFC_OFFLOAD;
    
    
    /* --- Prepare the Kernel Command --- */
    /* Resolve shared variables pointers to pass as argument */
    new_offload->kernelInfo.n_data = desc->n_data;
    
    if(new_offload->kernelInfo.n_data)
    {
        uint32_t fabricArgs;
        
        //Host array for fabric args
        new_offload->kernelInfo.host_args_ptr = (unsigned int *) p2012_socmem_alloc(new_offload->kernelInfo.n_data*sizeof(unsigned int), &fabricArgs);
        //Host array for host data pointers
        new_offload->kernelInfo.hdata_ptrs    = (unsigned int *)malloc(new_offload->kernelInfo.n_data*sizeof(unsigned int));
        
        //Fabric Args
        new_offload->kernelInfo.args = (unsigned int *) fabricArgs;
        
        data_desc_t *datad = (data_desc_t *) desc->data;
        new_offload->kernelInfo.data = (data_desc_t *) desc->data;
        for (i = 0; i < new_offload->kernelInfo.n_data; i++)
        {
            int data_size, data_ptr, data_type;
            uint32_t fabricCpy;
            
            data_type = datad[i].type;
            data_size = datad[i].size;
            data_ptr  = (uint32_t) datad[i].ptr;
            
            switch(data_type)
            {
                case 0:
                    //INOUT                    
                    new_offload->kernelInfo.hdata_ptrs[i] = (unsigned int)p2012_socmem_alloc(data_size, &fabricCpy);
                    new_offload->kernelInfo.host_args_ptr[i]  = fabricCpy;
                    memcpy((void *) new_offload->kernelInfo.hdata_ptrs[i], (void *) data_ptr ,data_size);
                    //printf("var %d val %d type %d\n", i, *((int *)(new_offload->kernelInfo.hdata_ptrs[i])), data_type);
                    break;
                    
                case 1:
                    //IN
                    new_offload->kernelInfo.hdata_ptrs[i] = (unsigned int)p2012_socmem_alloc(data_size, &fabricCpy);
                    new_offload->kernelInfo.host_args_ptr[i]  = fabricCpy;
                    memcpy((void *) new_offload->kernelInfo.hdata_ptrs[i], (void *) data_ptr ,data_size);
                    //printf("var %d val %d type %d\n", i, *((int *)(new_offload->kernelInfo.hdata_ptrs[i])), data_type);
                    break;
                    
                case 2:
                    //OUT
                    new_offload->kernelInfo.hdata_ptrs[i] = (unsigned int)p2012_socmem_alloc(data_size, &fabricCpy);
                    new_offload->kernelInfo.host_args_ptr[i]  = fabricCpy;
                    //printf("var %d val %d type %d\n", i, *((int *)(new_offload->kernelInfo.hdata_ptrs[i])), data_type);
                    break;
                    
                default:
                    //NONE
                    new_offload->kernelInfo.host_args_ptr[i] = (unsigned int) datad[i].ptr;
                    break;
            }            
        }
    }
    else
    {
        new_offload->kernelInfo.host_args_ptr = 0x0;
        new_offload->kernelInfo.args          = 0x0;
    }
    
    /* Attach Kernel Binary */
    new_offload->kernelInfo.binaryHandle = (int) desc->name;
    
    /* Set the number of clusters requested */
    new_offload->kernelInfo.n_clusters = desc->n_clusters;
    
    /* Update some more information */
    new_offload->hostUserPid         = (unsigned int ) new_offload | p2012_get_fabric_pid() | HOST_RT_PM_OMP_REQ_ID;
    new_offload->host_ptr            = (uint32_t ) new_offload;    
    
    *omp_offload = new_offload;
    
    return 0;
}

inline int gomp_offload_enqueue(uint32_t *fomp_offload){
    /* Finally enqueue the command */
    while (p2012_sendUserMessage(P2012_MAX_FABRIC_CLUSTER_COUNT, *fomp_offload) != 0);    
    return 0;
}