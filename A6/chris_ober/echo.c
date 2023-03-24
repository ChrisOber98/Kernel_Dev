#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/mutex.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chris Ober");

#define IOC_ECHO_RESET _IO(0x11, 0)
#define IOC_ECHO_GET_LENGTH _IOR(0x11, 1, size_t)

static dev_t device_number;
static struct cdev *cdev;
static struct class *echo_class;
static struct device *echo_device;

struct echo_private
{
       struct mutex mtx;
       char *buffer;
       size_t length;
};

static int echo_open(struct inode *inode, struct file *file)
{
       struct device *device = class_find_device_by_devt(echo_class, inode->i_rdev);
       struct echo_private *echo = dev_get_drvdata(device);
       file->private_data = echo;
       return 0;
}

static int echo_release(struct inode *inode, struct file *file)
{
       return 0;
}

static ssize_t echo_read(struct file *file, char __user *buf, size_t size, loff_t *off)
{
       ssize_t ret = 0;
       struct echo_private *private = file->private_data;
       if (mutex_lock_interruptible(&(private->mtx)))
              return -ERESTARTSYS;
       if(*off < 0 || *off >= private->length)
              goto out;
       size = min(size, (size_t)(private->length - *off));
       ret = -EFAULT;
       if(copy_to_user(buf, private->buffer + *off, size))
              goto out;
       ret = size;
       *off += size;
out:
       mutex_unlock(&(private->mtx));
       return ret;
}

static ssize_t echo_write(struct file *file, const char __user *ubuf, size_t size, loff_t *off)
{
       ssize_t ret;
       struct echo_private *private = file->private_data;
       char *buff = kmalloc(size, GFP_KERNEL);
       if(!buff)
              return -ENOMEM;
       ret = -EFAULT;


       if(copy_from_user(buff, ubuf, size))
              goto out;
       ret = -ERESTARTSYS;

       for (int i = 0, j = size - 1; i < j; i++, j--) {
        char temp = buff[i];
        buff[i] = buff[j];
        buff[j] = temp;
       }

       if (mutex_lock_interruptible(&(private->mtx)))
              goto out;
       swap(buff, private->buffer);
       private->length = size;
       *off = 0;
       mutex_unlock(&(private->mtx));
       ret = size;
out:
       kfree(buff);
       return ret;
}

static loff_t echo_llseek(struct file *file, loff_t offset, int whence)
{
       loff_t ret = -EINVAL, pos;
       struct echo_private *private = file->private_data;
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
              pos = private->length + offset;
              break;
       default:
              goto out;
       }
       if(pos < 0 || private->length < pos)
              goto out;
       file->f_pos = pos;
       ret = pos;
out:
       mutex_unlock(&(private->mtx));
       return ret;
}

long echo_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
       long ret = -EFAULT;
       struct echo_private *private = file->private_data;
       if(cmd != IOC_ECHO_RESET && cmd != IOC_ECHO_GET_LENGTH)
              return -EINVAL;
       if(mutex_lock_interruptible(&(private->mtx)))
              return -ERESTARTSYS;
       if(cmd == IOC_ECHO_GET_LENGTH) {
              if(copy_to_user((size_t __user *)arg, &(private->length), sizeof(size_t)))
                     goto out;
       } else {
              kfree(private->buffer);
              private->buffer = NULL;
              private->length = 0;
       }
       ret = 0;
out:
       mutex_unlock(&(private->mtx));
       return ret;

}

const static struct file_operations echo_fops = {
       .owner = THIS_MODULE,
       .open = echo_open,
       .release = echo_release,
       .read = echo_read,
       .write = echo_write,
       .llseek = echo_llseek,
       .unlocked_ioctl = echo_ioctl,
};

static char *echo_devnode(struct device *dev, umode_t *mode)
{
       if (mode)
               *mode = 0666;
       return NULL;
}

#define DEMO_NUM_MINORS 1

int __init echo_init(void)
{
       int ret;
       struct echo_private *echo;
       ret = alloc_chrdev_region(&device_number, 0, DEMO_NUM_MINORS, "echo");
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

       cdev_init(cdev, &echo_fops);
       ret = cdev_add(cdev, device_number, DEMO_NUM_MINORS);
       if (ret) {
               pr_err("Unable to add cdev %i\n", ret);
               goto err_cdev_add;
       }
       echo_class = class_create(THIS_MODULE, "echo");
       if (IS_ERR(echo_class)) {
               ret = PTR_ERR(echo_class);
               pr_err("Unable to create class for echo devices: %i\n", ret);
               goto err_cdev_add;
       }
       echo_class->devnode = echo_devnode;
       echo = kzalloc(sizeof(*echo), GFP_KERNEL);
       if (!echo) {
               ret = -ENOMEM;
               pr_err("Unable to create echo for echo devices: %i\n", ret);
               goto err_class_create;
       }
       mutex_init(&(echo->mtx));
       echo_device = device_create(echo_class, NULL, device_number, echo, "echo");
       if (IS_ERR(echo_device)) {
               ret = PTR_ERR(echo_device);
               pr_err("Unable to create device for echo devices: %i\n", ret);
               goto err_echo_allocate;
       }
       return 0;


err_echo_allocate:
               kfree(echo);
err_class_create:
               class_destroy(echo_class);
err_cdev_add:
               cdev_del(cdev);
err_cdev_alloc:
               unregister_chrdev_region(device_number, DEMO_NUM_MINORS);
err_alloc:
               return ret;
}

void __exit echo_exit(void)
{
       struct device *device = class_find_device_by_devt(echo_class, device_number);
       struct echo_private *echo = dev_get_drvdata(device);
       mutex_destroy(&(echo->mtx));
       kfree(echo);
       device_destroy(echo_class, device_number);
       class_destroy(echo_class);
       cdev_del(cdev);
       unregister_chrdev_region(device_number, DEMO_NUM_MINORS);
}

module_init(echo_init);
module_exit(echo_exit);
