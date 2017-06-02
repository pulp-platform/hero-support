#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <sys/mman.h>
//#include <sys/ioctl.h>

//#include <string.h>
#include <unistd.h>     // for usleep, access, close
#include <time.h>       // for time measurements

#define SVM_BASE_ADDR  0x1000000000UL
#define BRAM_BASE_ADDR 0xA0000000

#define SVM_SIZE_B  0xA00000
#define BRAM_SIZE_B 0x40000

#define PAGE_SIZE 0x1000
#define N_PAGES 64
#define SIZE_B (PAGE_SIZE * N_PAGES)
#define SIZE   (SIZE_B/4)

#define ARM_CLK_FREQ_MHZ 1200

double time_diff(const struct timespec start, const struct timespec end)
{
  const double _start = ((double)start.tv_sec)*1e9 + (double)start.tv_nsec;
  const double _end   = ((double)end.tv_sec)*1e9 + (double)end.tv_nsec;
  return (_end - _start) / 1e9;
}

int main(int argc, char *argv[]) {

  int i, fd, ret;
  struct timespec res, start, end;
  clock_getres(CLOCK_REALTIME, &res);

  printf("SMMU Interface User-Space Application\n");

  int mode;
  if (argc != 2) {
    printf("Number of arguments = %i\n", argc);
    mode = 0;
  }
  else {
    if ( (atoi(argv[1]) != 0) && (atoi(argv[1]) != 1) ) {
      printf("Invalid argument = %i\n", atoi(argv[1]));
      mode = 0;
    }
    else
      mode = atoi(argv[1]);
  }

  printf("Mode = %i\n", mode);

  if (mode == 0) {
    // create the user-space buffer
    unsigned int * buffer;
    ret = posix_memalign((void **)&buffer, (size_t)0x1000, (size_t)SIZE_B);
    if (ret) {
      printf("ERROR: Malloc failed for buffer.\n");
      return -ENOMEM;
    }

    printf("buffer @ %p\n", &buffer[0]);

    // init the buffer
    for (i = 0; i < SIZE; i++) {
      buffer[i] = i;
    }

    for (i = 0; i < 8; i++) {
      printf("buffer[%i] = %i\n", i, buffer[i]);
    }

    clock_gettime(CLOCK_REALTIME, &start);
    /*
     *  open the device
     */
    fd = open("/dev/SMMU_IF", O_RDWR | O_SYNC);
    if (fd < 0) {
      printf("ERROR: Opening failed.\n");
      return -ENOENT;
    }
    clock_gettime(CLOCK_REALTIME, &end);

    printf("SMMU IF device node opened.\n");
    printf("Opening took = %.6f s \n", time_diff(start, end));

    // write user-space offset
    ret = write(fd, (char *)&buffer, (size_t)sizeof(unsigned *));
    if (ret < 0) {
      printf("ERROR: Writing user-space offset failed.\n");
      return ret;
    }

    printf("Press ENTER to continue.\n");  
    getchar();    

    clock_gettime(CLOCK_REALTIME, &start);
    // close the file descriptor
    close(fd);
    fd = -1;
    clock_gettime(CLOCK_REALTIME, &end);

    printf("SMMU IF device node closed.\n");
    printf("Closing took = %.6f s \n", time_diff(start, end));

    // free memory
    free(buffer);
  }
  else { // mode != 0

    unsigned * bram_v_ptr;
    unsigned * svm_v_ptr;
    volatile unsigned tmp; 

    // open /dev/mem
    fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
      printf("ERROR: Opening failed \n");
      return -ENOENT;
    }

    // mmap PL space
    bram_v_ptr = (unsigned *)mmap(NULL, BRAM_SIZE_B, PROT_READ | PROT_WRITE, MAP_SHARED, fd, BRAM_BASE_ADDR);
    if (bram_v_ptr == MAP_FAILED) {
      printf("MMAP failed for PL space.\n");
      return -EIO;
    }

    svm_v_ptr = (unsigned *)mmap(NULL, SVM_SIZE_B, PROT_READ | PROT_WRITE, MAP_SHARED, fd, SVM_BASE_ADDR);
    if (svm_v_ptr == MAP_FAILED) {
      printf("MMAP failed for PL space.\n");
      return -EIO;
    }

    // write something to the PL space
    for (i = 0; i<16; i++) {
      *(bram_v_ptr + i) = 0xFF000000 + i;
    }

    // read from PL space
    for (i = 0; i<16; i++) {
      printf("bram @ %#lx: = %#x\n", (unsigned long)BRAM_BASE_ADDR + i*sizeof(unsigned), *(bram_v_ptr + i));
    }

    // read from PL space
    for (i = 0; i<16; i++) {
      printf("svm @ %#lx: = %#x\n", (unsigned long)SVM_BASE_ADDR + i*sizeof(unsigned), *(svm_v_ptr + i));
    }
    for (i = 1024; i<1024+16; i++) {
      printf("svm @ %#lx: = %#x\n", (unsigned long)SVM_BASE_ADDR + i*sizeof(unsigned), *(svm_v_ptr + i));
    }

    // measure page fault time
    clock_gettime(CLOCK_REALTIME, &start);
    for (i = 2; i<N_PAGES/2; i++) {
      tmp = *(unsigned *)((unsigned long)svm_v_ptr + i*PAGE_SIZE);
    }
    clock_gettime(CLOCK_REALTIME, &end);

    printf("Average page fault latency = %.6f s \n", time_diff(start, end)/(N_PAGES/2-2));
    printf("Average page fault latency = %.0f cycles \n",
      time_diff(start, end)/(N_PAGES/2-2)*ARM_CLK_FREQ_MHZ*1000000);

    // measure TLB miss time
    for (i = 0; i<N_PAGES/4; i++) {
      clock_gettime(CLOCK_REALTIME, &start);
      tmp = *(unsigned *)((unsigned long)svm_v_ptr + i*PAGE_SIZE);
      clock_gettime(CLOCK_REALTIME, &end);
      printf("L2 TLB hit latency = %.0f cycles \n",
      time_diff(start, end)*ARM_CLK_FREQ_MHZ*1000000);
    }

    for (i = 0; i<N_PAGES/4; i++) {
      clock_gettime(CLOCK_REALTIME, &start);
      tmp = *(unsigned *)((unsigned long)svm_v_ptr + i*PAGE_SIZE);
      clock_gettime(CLOCK_REALTIME, &end);
      printf("L1 TLB hit latency = %.0f cycles \n",
      time_diff(start, end)*ARM_CLK_FREQ_MHZ*1000000);
    }

    // unmap PL space
    ret = munmap(bram_v_ptr, 0x10000);
    if (ret) {
      printf("MUNMAP failed for PL space.\n");
    }
    ret = munmap(svm_v_ptr, 0x10000);
    if (ret) {
      printf("MUNMAP failed for PL space.\n");
    }

    // close the file descriptor
    close(fd);
    fd = -1;

  }

  return 0;
}
