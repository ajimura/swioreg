/* by Shuhei Ajimura */

#include <linux/ioctl.h>

#define DRV_NAME "swioreg"
#define CSR_BASE 0xff280800
#define CSR_SPAN 0x0100

struct swio_mem {
  unsigned int addr;
  unsigned int val;
};

#define IOC_MAGIC 'i'

#define SW_REG_READ  _IOR(IOC_MAGIC, 1, struct swio_mem)
#define SW_REG_WRITE _IOW(IOC_MAGIC, 2, struct swio_mem)
