/* by Shuhei Ajimura */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/sched.h>
//#include <linux/jiffies.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/ioctl.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <asm/io.h>
#include "swioreg.h"

#define VERB 0
#define MAX_REF 4

struct swioreg_devinfo {
  dev_t cdevno;
  struct cdev cdev;
};
struct swioreg_devinfo *swdevinfo = NULL;;

struct swioreg_dev {
  struct swioreg_devinfo *devinfo;
  void * __iomem csr_ptr;
  int major;
  int minor;
  int refid;
  unsigned long jiff_open;
};

int refcount;
static spinlock_t swioreg_spin_lock;

/* SW_OPEN */
static int swioreg_open(struct inode *inode, struct file *file)
{
  struct swioreg_dev *swioreg;
  unsigned int *csr;
  int major,minor;

  spin_lock(&swioreg_spin_lock);

  major=imajor(inode);
  minor=iminor(inode);

  if (refcount>=MAX_REF){
    printk(KERN_DEBUG DRV_NAME "%d:%d is opened too many\n",major,minor);
    spin_unlock(&swioreg_spin_lock);
    return -EBUSY;
  }else{
    refcount++;
  }

  printk(KERN_DEBUG DRV_NAME "_open(): %d:%d %lu/%d\n",major,minor,jiffies,HZ);

  swioreg = (struct swioreg_dev *)kzalloc(sizeof(struct swioreg_dev), GFP_KERNEL);
  if (!swioreg) {
    printk(KERN_DEBUG "Could not kzalloc() allocate memory.\n");
    goto fail_kzalloc;
  }
  file->private_data = swioreg;
  swioreg->devinfo=swdevinfo;

  csr=ioremap_nocache(CSR_BASE,CSR_SPAN);
  swioreg->csr_ptr=csr;

  swioreg->major=major;
  swioreg->minor=minor;

  swioreg->refid=refcount-1;

  swioreg->jiff_open=jiffies;

  spin_unlock(&swioreg_spin_lock);

  return 0;

 fail_kzalloc:
  spin_unlock(&swioreg_spin_lock);
  return -1;
}

/* SW_CLOSE */
static int swioreg_close(struct inode *inode, struct file *file)
{
  struct swioreg_dev *swioreg = file->private_data;

  spin_lock(&swioreg_spin_lock);

  printk(KERN_DEBUG DRV_NAME "_close(): %lu %lu/%d\n",
	 jiffies,jiffies-swioreg->jiff_open,HZ);
  iounmap(swioreg->csr_ptr);

  kfree(swioreg);

  refcount--;

  spin_unlock(&swioreg_spin_lock);

  return 0;
}

static long swioreg_ioctl(
  struct file *file,
  unsigned int cmd,
  unsigned long arg)
{
  struct swioreg_dev *swioreg = file->private_data;
  void * __iomem csrtop = swioreg->csr_ptr;

  struct swio_mem cmd_mem;

  unsigned long lock_flag;
  unsigned int address;

  int retval = 0;

  spin_lock_irqsave(&swioreg_spin_lock,lock_flag);

  if (!access_ok(VERIFY_READ, (void __user *)arg,_IOC_SIZE(cmd))){
    retval=-EFAULT; goto done; }
  if (copy_from_user(&cmd_mem, (int __user *)arg,sizeof(cmd_mem))){
    retval = -EFAULT; goto done; }

  switch(cmd){

  case SW_REG_READ:
    address=cmd_mem.addr & 0x000000ff;
    cmd_mem.val=ioread32(csrtop+address);
    rmb();
    if (copy_to_user((int __user *)arg, &cmd_mem, sizeof(cmd_mem))){
      retval = -EFAULT; goto done; }
#if VERB
    printk(KERN_DEBUG "(%d)IOR_cmd.val/addr %08x %08x(%s)\n",minor,cmd_mem.val,address, __func__);
#endif
    break;

  case SW_REG_WRITE:
    address=cmd_mem.addr & 0x000000ff;
    iowrite32(cmd_mem.val,csrtop+address);
    wmb();
#if VERB
    printk(KERN_DEBUG "(%d)IOW_cmd.val/addr %08x %08x(%s)\n",minor,cmd_mem.val,address, __func__);
#endif
    break;

  default:
    retval = -ENOTTY;
    goto done;
    break;
  }

  done:
    spin_unlock_irqrestore(&swioreg_spin_lock,lock_flag);
    return(retval);
}

/* character device file operations */
static struct file_operations swioreg_fops = {
  .owner = THIS_MODULE,
  .open = swioreg_open,
  .release = swioreg_close,
  .unlocked_ioctl = swioreg_ioctl,
};

/* swioreg_init() */
static int __init swioreg_init(void)
{
  int rc = 0;
  dev_t devno = MKDEV(0,0);

  printk(KERN_DEBUG DRV_NAME "_init(): %lu/%d\n",jiffies,HZ);
  printk(KERN_DEBUG "PAGESIZE=%ld\n", PAGE_SIZE);
  printk(KERN_DEBUG "HZ=%d\n", HZ);

  refcount=0;
  spin_lock_init(&swioreg_spin_lock);

  swdevinfo = (struct swioreg_devinfo *)kmalloc(sizeof(struct swioreg_devinfo), GFP_KERNEL);
  if (!swdevinfo) {
    printk(KERN_DEBUG "Could not kzalloc() allocate memory.\n");
    goto fail_kzalloc;
  }
  swdevinfo->cdevno=devno;

  rc = alloc_chrdev_region(&(swdevinfo->cdevno), 0/*requested minor base*/, 1/*count*/, DRV_NAME);
  if (rc < 0) {
    printk("alloc_chrdev_region() = %d\n", rc);
    goto fail_alloc;
  }
  cdev_init(&swdevinfo->cdev, &swioreg_fops);
  swdevinfo->cdev.owner = THIS_MODULE;
  rc = cdev_add(&swdevinfo->cdev, swdevinfo->cdevno, 1/*count*/);
  if (rc < 0) {
    printk("cdev_add() = %d\n", rc);
    goto fail_add;
  }
  printk(KERN_DEBUG "swioreg = %d:%d\n", MAJOR(swdevinfo->cdevno), MINOR(swdevinfo->cdevno));
  return 0;
 fail_add:
  /* free the dynamically allocated character device node */
  unregister_chrdev_region(swdevinfo->cdevno, 1/*count*/);
 fail_alloc:
 fail_kzalloc:
  return -1;
}

/* swioreg_exit() */
static void __exit swioreg_exit(void)
{
  printk(KERN_DEBUG DRV_NAME "_exit(): %lu/%d\n",jiffies,HZ);
  /* remove the character device */
  cdev_del(&swdevinfo->cdev);
  /* free the dynamically allocated character device node */
  unregister_chrdev_region(swdevinfo->cdevno, 1/*count*/);
  /* free mem */
  kfree(swdevinfo);
}

MODULE_LICENSE("Dual BSD/GPL");

module_init(swioreg_init);
module_exit(swioreg_exit);
