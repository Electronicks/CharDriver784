#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/slab.h>   //kmalloc(), kfree()
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/device.h>

#include <asm/atomic.h>
#include <asm/uaccess.h>

#define READWRITE_BUFSIZE 16
#define DEFAULT_BUFSIZE 256

MODULE_LICENSE("Dual BSD/GPL");

int buf_init(void);
void buf_exit(void);
/*
int buf_open(struct inode *inode, struct file *flip);
int buf_release(struct inode *inode, struct file *flip);
ssize_t buf_read(struct file *flip, char __user *ubuf, size_t count, loff_t *f_ops);
ssize_t buf_write(struct file *flip, const char __user *ubuf, size_t count, loff_t *f_ops);
long buf_ioctl(struct file *flip, unsigned int cmd, unsigned long arg);
*/
//module_init(buf_init);
//module_exit(buf_exit);

struct BufStruct {
    unsigned int    InIdx;
    unsigned int    OutIdx;
    unsigned short  BufFull;
    unsigned short  BufEmpty;
    unsigned int    BufSize;
    unsigned short  *Buffer;
} Buffer;

struct Buf_Dev {
    unsigned short      *ReadBuf;
    unsigned short      *WriteBuf;
    struct semaphore    SemBuf;
    unsigned short      numWriter;
    unsigned short      numReader;
    dev_t               dev;
    struct cdev         cdev;
} BDev;
/*
struct file_operations Buf_fops = {
    .owner      =   THIS_MODULE,
    .open       =   buf_open,
    .release    =   buf_release,
    .read       =   buf_read,
    .write      =   buf_write,
    .unlocked_ioctl =   buf_ioctl,
};
*/
char* DRIVER_NAME = "CircBuf";
#define READ_BUFFER_SIZE 64  //bytes
#define WRITE_BUFFER_SIZE 64 //bytes

__init int buf_init(void)
{
    int errno;
    printk(KERN_WARNING"CircBuf: Initialisation du pilote");

    //Obtention du numéro d'unité matériel
    errno = alloc_chrdev_region (&BDev.dev, 0, 1, DRIVER_NAME);
    if(errno = -1)
    {
        printk(KERN_ALERT"CircBuf: alloc_chrdev_region failed");
        //rien à désalouer
        return -1;
    }
    //tester errno

    sema_init(&BDev.SemBuf,1);
    BDev.ReadBuf = kmalloc(READ_BUFFER_SIZE, GFP_KERNEL);
    BDev.WriteBuf = kmalloc(WRITE_BUFFER_SIZE, GFP_KERNEL);
    BDev.numWriter = 0;
    BDev.numReader = 0;

    //Dernière chose
    errno = cdev_add(&BDev.cdev, BDev.dev, 1);
    if(errno = -1)
    {
        printk(KERN_ALERT"CircBuf: alloc_chrdev_region failed");
        buf_exit();
        return -1;
    }
    //tester errno
    return 0;
}

__exit void buf_exit(void)
//désallouer dans l'ordre inverse de l'allocation
{
    printk(KERN_WARNING"CircBuf: Fermeture du pilote");
    cdev_del(&BDev.cdev);
    kfree(BDev.WriteBuf);
    kfree(BDev.ReadBuf);
    //free semaphore, anything to do?
    unregister_chrdev_region (BDev.dev, 1);
}
/*
buf_open(struct inode *inode, struct file *flip)
{

}
buf_release(struct inode *inode, struct file *flip)
{

}
buf_read(struct file *flip, char __user *ubuf, size_t count, loff_t *f_ops)
{

}
buf_write(struct file *flip, const char __user *ubuf, size_t count, loff_t *f_ops)
{

}
buf_ioctl(struct file *flip, unsigned int cmd, unsigned long arg)
{

}
*/

int BufIn(struct BufStruct *Buf, unsigned short *Data) {
    if (Buf->BufFull)
        return -1;
    Buf->BufEmpty = 0;
    Buf->Buffer[Buf->InIdx] = *Data;
    Buf->InIdx = (Buf->InIdx + 1) % Buf->BufSize;
    if (Buf->InIdx == Buf->OutIdx)
        Buf->BufFull = 1;
    return 0;
}

int BufOut (struct BufStruct *Buf, unsigned short *Data) {
    if (Buf->BufEmpty)
        return -1;
    Buf->BufFull = 0;
    *Data = Buf->Buffer[Buf->OutIdx];
    Buf->OutIdx = (Buf->OutIdx + 1) % Buf->BufSize;
    if (Buf->OutIdx == Buf->InIdx)
        Buf->BufEmpty = 1;
    return 0;
}
