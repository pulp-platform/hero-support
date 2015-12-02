/* 
// Author: Juergen Gall, BIWI, ETH Zurich
// Email: gall@vision.ee.ethz.ch
*/

#pragma once

#include "CRTree.h"

#include <vector>

#ifdef PULP
extern "C" {
#include "pulp_func.h"
#include "time.h"
#include "zynq_pmm_user.h"
}
extern PulpDev* PULP_pulp;
extern int PULP_width;
extern int PULP_height;
#endif

class CRForest {
public:
	// Constructors
	CRForest(int trees = 0) {
		vTrees.resize(trees);
	}
	~CRForest() {
		for(std::vector<CRTree*>::iterator it = vTrees.begin(); it != vTrees.end(); ++it) delete *it;
		vTrees.clear();
	}

	// Set/Get functions
	void SetTrees(int n) {vTrees.resize(n);}
	int GetSize() const {return vTrees.size();}
	unsigned int GetDepth() const {return vTrees[0]->GetDepth();}
	unsigned int GetNumCenter() const {return vTrees[0]->GetNumCenter();}
	
	// Regression 
	void regression(std::vector<const LeafNode*>& result, uchar** ptFCh, int stepImg) const;

	// Training
	void trainForest(int min_s, int max_d, CvRNG* pRNG, const CRPatch& TrData, int samples);

	// IO functions
	void saveForest(const char* filename, unsigned int offset = 0);
	void loadForest(const char* filename, int type = 0);
	void show(int w, int h) const {vTrees[0]->showLeaves(w,h);}

	// Trees
	std::vector<CRTree*> vTrees;
};

inline void CRForest::regression(std::vector<const LeafNode*>& result, uchar** ptFCh, int stepImg) const {
  result.resize( vTrees.size() );

#ifdef PULP
  unsigned int n_failed = 0;
      
  // setup time measurement
  struct timespec res, tp1, tp2, tp3, tp4, tp5, tp6, tp7, tp8;
  float start, end, duration;
  
  clock_getres(CLOCK_REALTIME,&res);
  
  if (PULP_pulp) {
  
    // need pointer to PulpDev struct
    PulpDev * pulp;
    int width, height;
    
    pulp = PULP_pulp;
    width = PULP_width;
    height = PULP_height;

    clock_gettime(CLOCK_REALTIME,&tp1);

    // prepare offload
    int n_trees = this->GetSize();
    CRTree ** crtrees = (CRTree **)malloc(n_trees*sizeof(CRTree *));
    if (!crtrees) {
      printf("Malloc failed for crtrees.\n");
      return;
    }
    for (int i=0; i<n_trees; i++) {
      crtrees[i] = this->vTrees[i];
    }
    int n_features = 32; // hardcoded in CRPatch.cpp
  
    int * leaf_indices = (int *)malloc(n_trees*sizeof(int));
    if (!leaf_indices) {
      printf("Malloc failed for leaf_indices.\n");
      return;
    }
    memset((void *)leaf_indices, 0, (size_t)(n_trees*sizeof(int)));

    // set up a RAB slice for features pointers -- striping TOGETHER with miss_handling NOT SAVE!!!
    pulp_rab_req(pulp, (unsigned int)ptFCh, n_features*sizeof(unsigned int *), 0x3, 0x1, 0xFD, 0xFE);

    // set up striped RAB slices for patch
    unsigned s_height;
    pulp_rab_req_striped_mchan_img(pulp, 0x3, 1,
    				   (unsigned)height, (unsigned)width + (unsigned)stepImg,
    				   (unsigned)n_features, ptFCh, &s_height);

#define PRINT_ARGS
#define MEM_SHARING 3
#if MEM_SHARING == 1
    CRTree ** crtree_cv = (CRTree **)malloc(n_trees*sizeof(CRTree *));
    unsigned * crtree_cp = (unsigned *)malloc(n_trees*sizeof(unsigned));
    if (!crtree_cv || !crtree_cp)
      printf("Malloc failed for crtree_cv/crtree_cp.\n");

    int ** treetables_cv = (int **)malloc(n_trees*sizeof(int *));
    unsigned * treetables_cp = (unsigned *)malloc(n_trees*sizeof(unsigned));
    if (!treetables_cv || !treetables_cp)
      printf("Malloc failed for treetables_cv/treetables_cp.\n");

    unsigned * crtrees_cv;
    unsigned crtrees_cp;

    int * leaf_indices_cv;
    unsigned leaf_indices_cp;

    int size;
    
    // copy the crtrees to L3
    for (int i = 0; i<n_trees; i++) {
      // allocate memory for the CRTree structure
      crtree_cv[i] = (CRTree *)pulp_l3_malloc(pulp, sizeof(CRTree), &(crtree_cp[i]));
      printf("CRTree %d phys_addr = %#x\n",i,crtree_cp[i]);
      // copy the CRTree structure
      memcpy((void *)crtree_cv[i], (void *)crtrees[i], sizeof(CRTree));
     
      // allocate memory for the treetable
      size = crtrees[i]->num_nodes*7*sizeof(int);
      treetables_cv[i] = (int *)pulp_l3_malloc(pulp, size, &(treetables_cp[i]));
      printf("treetable %d phys_addr = %#x\n",i,treetables_cp[i]);
      // copy the treetable
      memcpy((void *)treetables_cv[i], (void *)crtrees[i]->treetable, size);

      // adjust pointers
      crtree_cv[i]->treetable = (int *)treetables_cp[i];
    }

    printf("Trees copied to contiguous L3.\n");
    
    // adjust crtrees
    crtrees_cv = (unsigned int *)pulp_l3_malloc(pulp, n_trees*sizeof(unsigned int *),&crtrees_cp);
    printf("crtrees phys_addr = %#x\n",crtrees_cp);
    for (int i = 0; i<n_trees; i++) {
      // write the physical address to the crtree vector in contiguous L3
      crtrees_cv[i] = crtree_cp[i];
      printf("crtree_cp[i] phys_addr = %#x\n",crtree_cp[i]);
    }

    // copy leaf_inidices
    leaf_indices_cv = (int *)pulp_l3_malloc(pulp, n_trees*sizeof(int), &leaf_indices_cp);
    printf("leaf_indices phys_addr = %#x\n",leaf_indices_cp);
    memset((void *)leaf_indices_cv, 0, (size_t)(n_trees*sizeof(int)));
#endif // MEM_SHARING

    // start offload
    pulp_write32(pulp->mailbox.v_addr, MAILBOX_WRDATA_OFFSET_B, 'b', PULP_START);
    
#if MEM_SHARING == 1
    // pass shared data elements to PULP
    pulp_write32(pulp->mailbox.v_addr, MAILBOX_WRDATA_OFFSET_B, 'b', n_trees);
    pulp_write32(pulp->mailbox.v_addr, MAILBOX_WRDATA_OFFSET_B, 'b', (unsigned int)crtrees_cp);

    pulp_write32(pulp->mailbox.v_addr, MAILBOX_WRDATA_OFFSET_B, 'b', width);
    pulp_write32(pulp->mailbox.v_addr, MAILBOX_WRDATA_OFFSET_B, 'b', height);
    pulp_write32(pulp->mailbox.v_addr, MAILBOX_WRDATA_OFFSET_B, 'b', stepImg);

    pulp_write32(pulp->mailbox.v_addr, MAILBOX_WRDATA_OFFSET_B, 'b', n_features);
    pulp_write32(pulp->mailbox.v_addr, MAILBOX_WRDATA_OFFSET_B, 'b', (unsigned int)ptFCh);

    pulp_write32(pulp->mailbox.v_addr, MAILBOX_WRDATA_OFFSET_B, 'b', (unsigned int)leaf_indices_cp); 

#ifdef PRINT_ARGS
    //printf("Wrote to mailbox: n_trees = %d\n",n_trees);
    printf("Wrote to mailbox: crtrees @ %#x\n",(unsigned int)crtrees_cp);

    //printf("Wrote to mailbox: width   = %d\n",width);
    //printf("Wrote to mailbox: height  = %d\n",height);
    //printf("Wrote to mailbox: stepImg = %d\n",stepImg);

    //printf("Wrote to mailbox: n_features   = %d\n",n_features);
    printf("Wrote to mailbox: features     @ %#x\n",(unsigned int)ptFCh);

    printf("Wrote to mailbox: leaf_indices @ %#x\n",(unsigned int)leaf_indices_cp);
#endif

#else
    // pass shared data elements to PULP
    pulp_write32(pulp->mailbox.v_addr, MAILBOX_WRDATA_OFFSET_B, 'b', n_trees);
    pulp_write32(pulp->mailbox.v_addr, MAILBOX_WRDATA_OFFSET_B, 'b', (unsigned int)crtrees);

    pulp_write32(pulp->mailbox.v_addr, MAILBOX_WRDATA_OFFSET_B, 'b', width);
    pulp_write32(pulp->mailbox.v_addr, MAILBOX_WRDATA_OFFSET_B, 'b', height);
    pulp_write32(pulp->mailbox.v_addr, MAILBOX_WRDATA_OFFSET_B, 'b', stepImg);

    pulp_write32(pulp->mailbox.v_addr, MAILBOX_WRDATA_OFFSET_B, 'b', n_features);
    pulp_write32(pulp->mailbox.v_addr, MAILBOX_WRDATA_OFFSET_B, 'b', (unsigned int)ptFCh);

    pulp_write32(pulp->mailbox.v_addr, MAILBOX_WRDATA_OFFSET_B, 'b', (unsigned int)leaf_indices);

#ifdef PRINT_ARGS
    //printf("Wrote to mailbox: n_trees = %d\n",n_trees);
    printf("Wrote to mailbox: crtrees @ %#x\n",(unsigned int)crtrees);

    //printf("Wrote to mailbox: width   = %d\n",width);
    //printf("Wrote to mailbox: height  = %d\n",height);
    //printf("Wrote to mailbox: stepImg = %d\n",stepImg);

    //printf("Wrote to mailbox: n_features   = %d\n",n_features);
    printf("Wrote to mailbox: features     @ %#x\n",(unsigned int)ptFCh);

    printf("Wrote to mailbox: leaf_indices @ %#x\n",(unsigned int)leaf_indices);
#endif
#endif // MEM_SHARING

    printf("crrees[0]->treetable[0] = %#x @ %#x \n",
	   (unsigned int)(crtrees[0]->treetable[0]),(unsigned int)(&(crtrees[0]->treetable[0])));
    printf("crrees[0]->treetable[6] = %#x @ %#x \n",
	   (unsigned int)(crtrees[0]->treetable[6]),(unsigned int)(&(crtrees[0]->treetable[6])));

    clock_gettime(CLOCK_REALTIME,&tp2);
 
#define DMA_CHECK 1
#if DMA_CHECK == 1
    // check DMA transfer
    unsigned int tcdm_patch;
    unsigned char pixel_l3, pixel_tcdm;
    unsigned int word_tcdm, offset_tcdm;
        
    pulp_mailbox_read(pulp, &tcdm_patch, 1);
    if (tcdm_patch >= 0x10000000 && tcdm_patch <= 0x10012000) {
      printf("Patch in TCDM @ %#x.\n",tcdm_patch);

      printf("DMA Check: Starting comparison.\n");
      for (int i=0; i<n_features; i++) {
	for (int j=0; j<height; j++) {
	  for (int k=0; k<width; k++) {
	    pixel_l3 = *(ptFCh[i]+j*stepImg+k);
	    offset_tcdm = i*(width*height)+j*width+k;
	    word_tcdm = pulp_read32(pulp->clusters.v_addr, tcdm_patch-0x10000000+offset_tcdm, 'b');
	    if (BF_GET(offset_tcdm,0,2) == 0)
	      pixel_tcdm = (unsigned char)BF_GET(word_tcdm,0,8);
	    else if (BF_GET(offset_tcdm,0,2) == 1)
	      pixel_tcdm = (unsigned char)BF_GET(word_tcdm,8,8);
	    else if (BF_GET(offset_tcdm,0,2) == 2)
	      pixel_tcdm = (unsigned char)BF_GET(word_tcdm,16,8);
	    else // (BF_GET(offset_tcdm,0,2) == 3)
	      pixel_tcdm = (unsigned char)BF_GET(word_tcdm,24,8);

	    if ( pixel_l3 != pixel_tcdm) {
	      n_failed++;
	      if (n_failed < 100) {
		printf("Channel %i, Height %i, Width %i @ %#x | %#x: L3 = %#x, TCDM = %#x\n",
		       i,j,k,ptFCh[i]+j*stepImg+k, tcdm_patch+i*width*height+j*width+k ,pixel_l3, pixel_tcdm);
	      
	      }
	    }
	  }
	}
      }
      printf("Comparison failed for %i pixels.\n",n_failed);
        
      pulp_write32(pulp->mailbox.v_addr, MAILBOX_WRDATA_OFFSET_B, 'b', HOST_READY);
    }
#endif // DMA_CHECK

    // wait for offload end
    unsigned int ret;
    pulp_mailbox_read(pulp, &ret, 1);
    if (ret != PULP_DONE) {
      printf("ERROR: PULP execution failed.\n");
      //return;
      PULP_pulp = 0;
    }

#define PRINT_STDOUT
#ifdef PRINT_STDOUT
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
#endif // PRINT_STDOUT

    clock_gettime(CLOCK_REALTIME,&tp5);

    pulp_rab_free_striped(pulp);
    pulp_rab_free(pulp, 0xFE);
 
    // assign result[i]
    //#define PROFILE
#ifndef PROFILE
    printf("PULP result[.]: ");
    for (int i=0; i<n_trees; i++) {
#if MEM_SHARING != 1
      result[i] = &(vTrees[i]->leaf[leaf_indices[i]]);
#else
      result[i] = &(vTrees[i]->leaf[leaf_indices_cv[i]]);
#endif // MEM_SHARING
      printf("%#x\t",result[i]);
    }
    printf("\n");
#endif // PROFILE

    // cleanup
    free(crtrees);
    free(leaf_indices);

    clock_gettime(CLOCK_REALTIME,&tp6);
  }

  //#else
  // for (int i=0; i<(int)vTrees.size(); i++) {
  //   result[i] = 0;
  //   printf("Clear result[%d] = %#x\n",i,result[i]);
  // }

#ifndef PROFILE
  if (PULP_pulp)
    printf("ARM  result[.]: "); 
#endif

  /*************************************************************************/
  //#define ZYNQ_PMM 1
#if ZYNQ_PMM == 1
  int ret;
  int *zynq_pmm_fd;
  int zynq_pmm;

  char proc_text[ZYNQ_PMM_PROC_N_CHARS_MAX];
  long long counter_values[(N_ARM_CORES+1)*2];
  double cache_miss_rates[(N_ARM_CORES+1)*2];
  
  if (PULP_pulp) {
    // setup cache miss rate measurement
    zynq_pmm_fd = &zynq_pmm;
    ret = zynq_pmm_open(zynq_pmm_fd);
    if (ret)
      return;

    // delete the accumulation counters
    ret = zynq_pmm_parse(proc_text, counter_values, -1);

    // reset cache counters
    zynq_pmm_read(zynq_pmm_fd,proc_text);
  }
#endif // ZYNQ_PMM
  /*************************************************************************/

  clock_gettime(CLOCK_REALTIME,&tp7);

  // the original function - for verification
  for(int i=0; i<(int)vTrees.size(); ++i) {
    result[i] = vTrees[i]->regression(ptFCh, stepImg);
#ifndef PROFILE
#ifdef PULP
    if (PULP_pulp)
      printf("%#x\t",result[i]);
#endif
#endif // PROFILE
  }

  clock_gettime(CLOCK_REALTIME,&tp8);

#ifndef PROFILE
  if (PULP_pulp)
    printf("\n");
#endif

  /*************************************************************************/
  // print cache miss rate
#if ZYNQ_PMM == 1 
  if (PULP_pulp) {
    zynq_pmm_read(zynq_pmm_fd, proc_text);
    zynq_pmm_parse(proc_text, counter_values, 1); // accumulate cache counter values

    zynq_pmm_compute_rates(cache_miss_rates, counter_values); // compute cache miss rates
    double miss_rate_0 = cache_miss_rates[0]*100;
    double miss_rate_1 = cache_miss_rates[1]*100;
    printf("\n###########################################################################\n");
    printf("Host Kernel L1 D-Cache Miss Rates: %.2f %% (Core 0), %.2f %% (Core 1)\n",miss_rate_0,miss_rate_1);
    printf("\n###########################################################################\n");

    zynq_pmm_close(zynq_pmm_fd);
  }
#endif // ZYNQ_PMM
  /*************************************************************************/

  if (n_failed)
    PULP_pulp = 0;

#ifdef PROFILE
  if (PULP_pulp)  {
    start = ((double)tp7.tv_sec)*1000000000 + (double)tp7.tv_nsec; 
    end = ((double)tp8.tv_sec)*1000000000 + (double)tp8.tv_nsec; 
    duration = (end - start)/1000000000;
    printf("Host execution time = %.6f s \n",duration);

    start = ((double)tp1.tv_sec)*1000000000 + (double)tp1.tv_nsec; 
    end = ((double)tp2.tv_sec)*1000000000 + (double)tp2.tv_nsec; 
    duration = (end - start)/1000000000;
    printf("Offload out time    = %.6f s \n",duration);

    //start = ((double)tp3.tv_sec)*1000000000 + (double)tp3.tv_nsec; 
    //end = ((double)tp4.tv_sec)*1000000000 + (double)tp4.tv_nsec; 
    //duration = (end - start)/1000000000;
    //printf("Execution time      = %.6f s \n",duration);

    start = ((double)tp5.tv_sec)*1000000000 + (double)tp5.tv_nsec; 
    end = ((double)tp6.tv_sec)*1000000000 + (double)tp6.tv_nsec; 
    duration = (end - start)/1000000000;
    printf("Offload in time     = %.6f s \n",duration);
  }
#endif // PROFILE

#else  // PULP
  /*************************************************************************/
  // the original function
  for(int i=0; i<(int)vTrees.size(); ++i) {
    result[i] = vTrees[i]->regression(ptFCh, stepImg);
  }
  /*************************************************************************/
#endif // PULP
}

//Training
inline void CRForest::trainForest(int min_s, int max_d, CvRNG* pRNG, const CRPatch& TrData, int samples) {
	for(int i=0; i < (int)vTrees.size(); ++i) {
		vTrees[i] = new CRTree(min_s, max_d, TrData.vLPatches[1][0].center.size(), pRNG);
		vTrees[i]->growTree(TrData, samples);
	}
}

// IO Functions
inline void CRForest::saveForest(const char* filename, unsigned int offset) {
	char buffer[200];
	for(unsigned int i=0; i<vTrees.size(); ++i) {
		sprintf_s(buffer,"%s%03d.txt",filename,i+offset);
		vTrees[i]->saveTree(buffer);
	}
}

inline void CRForest::loadForest(const char* filename, int type) {
	char buffer[200];
	for(unsigned int i=0; i<vTrees.size(); ++i) {
		sprintf_s(buffer,"%s%03d.txt",filename,i);
		vTrees[i] = new CRTree(buffer);
	}
}