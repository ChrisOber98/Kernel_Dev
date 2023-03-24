#include <linux/module.h>
#include <linux/init.h>
#include <linux/mutex.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/cdev.h>

MODULE_AUTHOR("Chris Ober");
MODULE_LICENSE("GPL");

static dev_t device_number;
static struct cdev *cdev;
static struct class *igpay_class;
static struct device *igpay_device;

#define IOC_PIG_ENCODE_MSG _IO(0x11, 0)
#define IOC_PIG_GET_ORIG   _IOR(0x11, 1, struct pig_ioctl_args)
#define IOC_PIG_RESET      _IO(0x11, 2)
#define IOC_PIG_MSG_LEN    _IOR(0x11, 3, size_t)
#define IOC_PIG_NUM_TRANS  _IOR(0x11, 4, size_t)

struct pig_ioctl_args {
	char *buff;
	size_t buff_sz;
};

struct igpay_private
{
       struct mutex mtx;

       char *og_buffer;
       size_t og_length;

       char *encode_buffer;
       size_t encode_length;

       int num_trans;
};

static int igpay_open(struct inode *inode, struct file *file)
{
       struct device *device = class_find_device_by_devt(igpay_class, inode->i_rdev);
       struct igpay_private *igpay = dev_get_drvdata(device);
       file->private_data = igpay;
       return 0;
}

static int igpay_release(struct inode *inode, struct file *file)
{
       return 0;
}

// file = fd for file, ubuff = where data is going to be written, size = number of bytes to read, off = pointer to file position
static ssize_t igpay_read(struct file *file, char __user *buf, size_t size, loff_t *off)
{
       //initialze return variable
       ssize_t ret = 0;

       //define character struct
       struct igpay_private *private = file->private_data;

       //Lock mutex
       if (mutex_lock_interruptible(&(private->mtx)))
              return -ERESTARTSYS;

       //check if offset is outside length of buffer
       if(*off < 0 || *off >= private->encode_length)
              goto out;

       //set size of max number of bytes that can be read
       size = min(size, (size_t)(private->encode_length - *off));

       //If it gets past this then error will be a Bad adress = EFAULT
       ret = -EFAULT;

       //Copy buffer from kernel to userspace
       if(copy_to_user(buf, private->encode_buffer + *off, size))
              goto out;

       ret = size;
       *off += size;
out:
       mutex_unlock(&(private->mtx));
       return ret;
}

//file = fd for file, ubuf = data to be written, size = return value number of bytes to be written, off = current file position
static ssize_t igpay_write(struct file *file, const char __user *ubuf, size_t size, loff_t *off)
{

       //define ret as variable to store return value
       ssize_t ret;

       //define character struct
       struct igpay_private *private = file->private_data;

       //Malloc a buffer
       char *buff = kmalloc(size , GFP_KERNEL);
       char *encode_buff = kmalloc(size + 4, GFP_KERNEL);

       int _size = strlen(encode_buff) + 2;

       char first = encode_buff[0];

       //check if buffer is NULL
       if (ubuf == NULL) {
              return -EINVAL;
       }


       //Error check for failed alloc (ENOMEM = cannot allocate memory)
       if(!buff)
              return -ENOMEM;

       //If it gets past this then error will be a Bad adress = EFAULT
       ret = -EFAULT;

       //Copy buffer from userspace to kernelspace
       if(copy_from_user(buff, ubuf, size))
              //Goto out and return ret from above for reason above
              goto out;

       if(copy_from_user(encode_buff, ubuf, size))
              //Goto out and return ret from above for reason above
              goto out;

       //Change error to ERESTARTSYS
       ret = -ERESTARTSYS;

       //encode piglatin

       for (int i = 0; i < _size + 1; i++)
       {
              if(i < _size - 3)
              {
                     encode_buff[i] = buff[i + 1];
                     continue;
              }
              if (i == _size - 3)
              {
                     encode_buff[i] = first;
                     continue;
              }
              if (i == _size - 2)
              {
                     encode_buff[i] = 'a';
                     continue;
              }
              if (i == _size - 1)
              {
                     encode_buff[i] = 'y';
                     continue;
              }
              if (i == _size)
              {
                     encode_buff[i] = '\0';
                     continue;
              }
       }

       private->num_trans += 1;

       //Lock mutex
       if (mutex_lock_interruptible(&(private->mtx)))
              //if fails send error message above
              goto out;

       //Place what is in buffer to the character device buffer, and size
       swap(buff, private->og_buffer);
       private->og_length = size;

       swap(encode_buff, private->encode_buffer);
       private->encode_length = _size + 1;

       //set offset to 0
       *off = 0;

       //unlock mutex
       mutex_unlock(&(private->mtx));

       //return size
       ret = size;
out:
       //Error handling free buffer and send whatever error message is in ret
       kfree(buff);
       return ret;
}

static loff_t igpay_llseek(struct file *file, loff_t offset, int whence)
{
       loff_t ret = -EINVAL, pos;
       struct igpay_private *private = file->private_data;
       if (mutex_lock_interruptible(&(private->mtx)))
              return -ERESTARTSYS;
       switch(whence)
       {
       case SEEK_SET:
              pos = offset;
              break;
       case SEEK_CUR:
              pos = file->f_pos + offset;
              break;
       case SEEK_END:
              pos = private->encode_length + offset;
              break;
       default:
              goto out;
       }
       if(pos < 0 || private->encode_length < pos)
              goto out;
       file->f_pos = pos;
       ret = pos;
out:
       mutex_unlock(&(private->mtx));
       return ret;
       return 0;
}

long igpay_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
       struct pig_ioctl_args args;

       long ret = -EFAULT;
       struct igpay_private *private = file->private_data;
       if(cmd != IOC_PIG_ENCODE_MSG && cmd != IOC_PIG_GET_ORIG && cmd != IOC_PIG_RESET && cmd != IOC_PIG_MSG_LEN && cmd != IOC_PIG_NUM_TRANS)
       {
              ret = -EINVAL;
              goto out;
       }
       if(mutex_lock_interruptible(&(private->mtx)))
              return -ERESTARTSYS;

       switch (cmd) {
              // get inital message return it through pig_ioctl_args
              case IOC_PIG_GET_ORIG:
                     printk("I Have entered correctly into the case statment\n");
                     //Checks to make sure pig_ioctl_args are correct
                     if (copy_from_user(&args, (struct pig_ioctl_args *)arg, sizeof(struct pig_ioctl_args)))
                     {
                            ret = -EFAULT;
                            goto out;
                     }
                     printk("I Have copied from user in Ioctl\n");
                     if (args.buff == NULL)
                     {
                            printk("buff == NULL in Ioctl\n");
                            ret = -EINVAL;
                            goto out;
                     }
                     if (args.buff_sz < private->og_length)
                     {
                            printk("size < in Ioctl\n");
                            ret = -EINVAL;
                            goto out;
                     }
                     if (copy_to_user(args.buff, private->og_buffer, private->og_length))
                     {
                            ret = -EFAULT;
                            goto out;
                     }
                     printk("I Have copied to user in Ioctl\n");
                     ret = 0;
                     break;
              case IOC_PIG_ENCODE_MSG:
                     int size = strlen(private->encode_buffer) + 2;

                     char first = private->encode_buffer[0];

                     char *og_encode = private->encode_buffer;

                     private->encode_buffer = kmalloc(size + 4, GFP_KERNEL);

                     printk("first char %c size %d encode buffer %s private biff %s\n", first, size, og_encode, private->encode_buffer);

                     for (int i = 0; i < size + 1; i++)
                     {
                            if(i < size - 3)
                            {
                                   private->encode_buffer[i] = og_encode[i + 1];
                                   continue;
                            }
                            if (i == size - 3)
                            {
                                   private->encode_buffer[i] = first;
                                   continue;
                            }
                            if (i == size - 2)
                            {
                                   private->encode_buffer[i] = 'a';
                                   continue;
                            }
                            if (i == size - 1)
                            {
                                   private->encode_buffer[i] = 'y';
                                   continue;
                            }
                            if (i == size)
                            {
                                   private->encode_buffer[i] = '\0';
                                   continue;
                            }
                            printk("private encode[%d] = %s", i, private->encode_buffer);
                     }
                     private->num_trans += 1;
                     private->encode_length = size + 1;
                     ret = 0;
                     break;
              case IOC_PIG_MSG_LEN:
                     ssize_t len;
                     if (copy_from_user(&len, (ssize_t *)arg, sizeof(arg)))
                     {
                            ret = -EFAULT;
                            goto out;
                     }

                     len = private->encode_length;

                     if (copy_to_user((ssize_t *)arg, &len, sizeof(arg))) {
                            ret = -EFAULT;
                            goto out;
                     }
                     ret = 0;
                     break;
              case IOC_PIG_RESET:
                     memset(private->encode_buffer, '\0', sizeof(private->encode_buffer));
                     private->encode_length = 0;

                     memset(private->og_buffer, '\0', sizeof(private->og_buffer));
                     private->og_length = 0;

                     private->num_trans = 0;

                     ret = 0;
                     break;
              case IOC_PIG_NUM_TRANS:
                     ssize_t trans;
                     if (copy_from_user(&trans, (ssize_t *)arg, sizeof(arg)))
                     {
                            ret = -EFAULT;
                            goto out;
                     }

                     trans = private->num_trans;

                     if (copy_to_user((ssize_t *)arg, &trans, sizeof(arg))) {
                            ret = -EFAULT;
                            goto out;
                     }
                     ret = 0;
                     break;
              default:
                     ret = -EINVAL;
                     goto out;
       }
out:
       mutex_unlock(&(private->mtx));
       return ret;
}

const static struct file_operations igpay_fops = {
       .owner = THIS_MODULE,
       .open = igpay_open,
       .release = igpay_release,
       .read = igpay_read,
       .write = igpay_write,
       .llseek = igpay_llseek,
       .unlocked_ioctl = igpay_ioctl,
};

static char *igpay_devnode(struct device *dev, umode_t *mode)
{
       if (mode)
               *mode = 0666;
       return NULL;
}

#define DEMO_NUM_MINORS 1

static int __init igpay_init(void)
{
       int ret;
       struct igpay_private *igpay;
       ret = alloc_chrdev_region(&device_number, 0, DEMO_NUM_MINORS, "igpay");
       if (ret) {
               pr_err("Unable to allocate chrdev region: %i\n", ret);
               goto err_alloc;
       }

       cdev = cdev_alloc();
       if (!cdev) {
               ret = -ENOMEM;
               pr_err("Unable to allocate cdev: %i\n", ret);
               goto err_cdev_alloc;
       }

       cdev_init(cdev, &igpay_fops);
       ret = cdev_add(cdev, device_number, DEMO_NUM_MINORS);
       if (ret) {
               pr_err("Unable to add cdev %i\n", ret);
               goto err_cdev_add;
       }
       igpay_class = class_create(THIS_MODULE, "igpay");
       if (IS_ERR(igpay_class)) {
               ret = PTR_ERR(igpay_class);
               pr_err("Unable to create class for echo devices: %i\n", ret);
               goto err_cdev_add;
       }
       igpay_class->devnode = igpay_devnode;
       igpay = kzalloc(sizeof(*igpay), GFP_KERNEL);
       if (!igpay) {
               ret = -ENOMEM;
               pr_err("Unable to create echo for echo devices: %i\n", ret);
               goto err_class_create;
       }
       mutex_init(&(igpay->mtx));
       igpay_device = device_create(igpay_class, NULL, device_number, igpay, "igpay");
       if (IS_ERR(igpay_device)) {
               ret = PTR_ERR(igpay_device);
               pr_err("Unable to create device for echo devices: %i\n", ret);
               goto err_igpay_allocate;
       }
       return 0;


err_igpay_allocate:
               kfree(igpay);
err_class_create:
               class_destroy(igpay_class);
err_cdev_add:
               cdev_del(cdev);
err_cdev_alloc:
               unregister_chrdev_region(device_number, DEMO_NUM_MINORS);
err_alloc:
               return ret;
}

static void __exit igpay_exit(void)
{
	struct device *device = class_find_device_by_devt(igpay_class, device_number);
       struct igpay_private *igpay = dev_get_drvdata(device);
       mutex_destroy(&(igpay->mtx));
       kfree(igpay);
       device_destroy(igpay_class, device_number);
       class_destroy(igpay_class);
       cdev_del(cdev);
       unregister_chrdev_region(device_number, DEMO_NUM_MINORS);
}

module_init(igpay_init);
module_exit(igpay_exit);
