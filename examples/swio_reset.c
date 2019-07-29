#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "swioreg.h"

int main(int argc, char* argv[]){
  int st;
  int fd;
  if ((fd=swio_open())<0){
    printf("open error\n");
    exit(-1);
  }
  swio_reset();
  swio_close();
}
