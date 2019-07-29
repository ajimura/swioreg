/* by Shuhei Ajimura */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define DevName "/dev/swioreg0"

#define DEF_EVTAG 255

#define ADD_R_TAG 0x00000000
#define ADD_R_TOT 0x00000004
#define ADD_R_WAD 0x00000008
#define ADD_R_RAD 0x0000000C
#define ADD_R_FUL 0x00000010
#define ADD_W_RDN 0x00000000
#define ADD_W_RST 0x00000004

int swio_open();
void swio_close();
int swio_w(unsigned int /*addr*/, unsigned int /*data*/);
int swio_r(unsigned int /*addr*/, unsigned int */*data*/);
int swio_reset();
int swio_read(unsigned char */*tag*/);
int swio_done();
int swio_show();

struct swio_mem {
  unsigned int addr;
  unsigned int val;
};
#define IOC_MAGIC 'i'
#define SW_REG_READ  _IOR(IOC_MAGIC, 1, struct swio_mem)
#define SW_REG_WRITE _IOW(IOC_MAGIC, 2, struct swio_mem)

int swio_fd;

int swio_open()
{
  //  int fd;
  //  if ((fd=open(DevName,O_RDWR))<0){
  //    printf("error: cannot open device file.\n");
  //  }
  //  return(fd);
  swio_fd=open(DevName,O_RDWR);
  return(swio_fd);
}

void swio_close()
{
  close(swio_fd);
}

int swio_w(unsigned int addr, unsigned int data)
{
  struct swio_mem swio;
  int ret;
  swio.addr=addr;
  swio.val=data;
  ret=ioctl(swio_fd,SW_REG_WRITE,&swio);
  return(ret);
}

int swio_r(unsigned int addr, unsigned int *data)
{
  struct swio_mem swio;
  int ret;
  swio.addr=addr;
  ret=ioctl(swio_fd,SW_REG_READ,&swio);
  *data=swio.val;
  return(ret);
}

int swio_reset()
{
  return(swio_w(ADD_W_RST,0));
}

int swio_read(unsigned char *tag)
{
  unsigned int data;
  int ret;
  ret=swio_r(ADD_R_TAG,&data);
  *tag=data;
  return(ret);
}

int swio_done()
{
  return(swio_w(ADD_W_RDN,0));
}

int swio_show()
{
  unsigned int tag,tot,wad,rad,ful;
  swio_r(ADD_R_TAG,&tag);
  swio_r(ADD_R_TOT,&tot);
  swio_r(ADD_R_WAD,&wad);
  swio_r(ADD_R_RAD,&rad);
  swio_r(ADD_R_FUL,&ful);

  printf("Current Event Tag:\t0x%02X\n",tag);
  printf("Trig Count:\t\t0x%02X\n",tot);
  printf("Buf Write Pointer:\t0x%02X\n",wad);
  printf("Buf Read Pointer:\t0x%02X\n",rad);
  printf("Buf Full:\t\t0x%02X\n",ful);
  return(0);
}
