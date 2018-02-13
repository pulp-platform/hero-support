#include <fcntl.h>    // open()
#include <unistd.h>   // write()
#include <string.h>
#include <asm/ioctls.h>
#include <asm/termbits.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>

#include <errno.h>

int main(int argc, char *argv[]) {

  char buffer[256];
  struct termios2 tio;
  int fd;
  unsigned int i;
  char c;

  int baudrate = 115200; // PULP default
  int read_uart = 1;
  
  if (argc < 2) {
    printf("*** PULP UART Reader ***\n");
  }
  else {
    baudrate = atoi(argv[1]);
    printf("Configuring for baudrate %d\n",baudrate);
    
    if (argc > 2)
      read_uart = atoi(argv[2]);
    
    if (argc > 3)
      printf("WARNING: More than 2 command line arguments are not supported. Those will be ignored.\n");
  }

  if (( fd = open("/dev/ttyPS1", O_RDONLY | O_NOCTTY) ) < 0) {
    printf("Failed to open /dev/ttyPS1.\n");
    return 1;
  }

  // set baudrate, PULP by default outputs at 115200 baud
  ioctl(fd, TCGETS2, &tio);
  tio.c_cflag &= ~CBAUD;
  tio.c_cflag |= BOTHER;
  tio.c_ispeed = baudrate;
  tio.c_ospeed = baudrate;

  if (ioctl(fd, TCSETS2, &tio) != 0) {
    printf("Failed to set baudrate.\n");
    return 1;
  }
   
  if (read_uart) {
    printf("Will now start to read from /dev/ttyPS1. Hit Ctrl+C to exit.\n\n");

    i = 0;
    while (1) {
      int n = read(fd, &c, 1);
  
      if (n == EINTR) {
        // we were interrupted, so we assume we should exit
        break;
  
      } else if (n < 0) {
        printf("Read failed!\n");
  
      } else if (n > 0) {
        if (i == (sizeof(buffer) - 1) || c == '\n') {
          buffer[i] = '\0';
          printf("%s\n", buffer);
          i = 0;
        } else {
          buffer[i] = c;
          i++;
        }
      }
    }
  }
  close(fd);

  return 0;
}