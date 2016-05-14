#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/un.h>
#include <string.h>
//#include <netinet/in.h>

#define SHMEM_PATH "/my_shmem"
#define SHMEM_SIZE (1 << 16)
#define SERV_PATH "./server.soc"

int main() {
  int res, shmem_fd;
  void *ptr;
  int temp_s, s;
  struct sockaddr_un addr;
  int signal[1];
  signal[0] = 1;


  shmem_fd = shm_open(SHMEM_PATH, O_RDWR | O_CREAT, 0666);
  if (shmem_fd < 0) {
    perror("shm_open");
    exit(1);
  }

  res = ftruncate(shmem_fd, SHMEM_SIZE);
  if (res) {
    perror("ftruncate");
    exit(1);
  }

  
  ptr = mmap(NULL, SHMEM_SIZE,
             PROT_READ | PROT_WRITE,
             MAP_SHARED, shmem_fd, 0);
  if (!ptr) {
    perror("mmap");
    exit(1);
  }

  s = socket(AF_UNIX, SOCK_STREAM, 0);
  if (s < 0) {
    perror("socket");
    exit(1);
  }
  
  addr.sun_family = AF_UNIX;
  strcpy(addr.sun_path, SERV_PATH);
  if (bind (s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind");
    exit(1);
  }

    
  if (listen(s, 1)) {
    perror("listen");
    res = unlink(SERV_PATH);
    if (res) {
    perror("unlink");
    res = close(s);
    if (res) {
        perror("close");
        exit(1);
      }
    }
    res = close(s);
    if (res) {
      perror("close");
      exit(1);
    }
    exit(1);
  }
  
  

  temp_s = accept(s, NULL, NULL);
  if (temp_s < 0) {
    perror("accept");
    res = unlink(SERV_PATH);
    if (res) {
    perror("unlink");
    res = close(s);
    if (res) {
      perror("close");
      exit(1);
    }
  }
    res = close(s);
    if (res) {
      perror("close");
      exit(1);
    }
    exit(1);
  }
  if (recv(temp_s, signal, 4, 0) > 0) {
    printf("%d\n", *((int*) ptr));
  }
  

  res = unlink(SERV_PATH);
  if (res) {
    perror("unlink");
    res = close(s);
    if (res) {
      perror("close");
      exit(1);
    }
  }
  res = close(temp_s);
  if (res) {
    perror("close");
    exit(1);
  }
  res = close(s);
  if (res) {
    perror("close");
    exit(1);
  }  

  res = munmap(ptr, SHMEM_SIZE);
  if (res) {
    perror("munmap");
    exit(1);
  }

  res = close(shmem_fd);
  if (res) {
    perror("close");
    exit(1);
  }

  res = shm_unlink(SHMEM_PATH);
  if (res) {
    perror("shm_unlink");
    exit(1);
  }
  return 0;
}
