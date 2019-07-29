#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "swioreg.h"

int main(int argc, char* argv[]){
  int st;
  int fd;
  unsigned char data;
  if ((fd=swio_open())<0){
    printf("open error\n");
    exit(-1);
  }
  swio_read(&data);
  printf("Read data: %02x\n",data);
  swio_close();
}
