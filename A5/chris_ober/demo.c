#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chris Ober");

static dev_t device_number;
static struct cdev *cdev;
static struct class *demo_class;
static struct device *demo_device;

struct demo_operation_counter
{
       int operation_counter;
       int file_counter;
};

static int demo_open(struct inode *inode, struct file *file)
{
       struct device *device = class_find_device_by_devt(demo_class, inode->i_rdev);
       struct demo_operation_counter *counter = dev_get_drvdata(device);
       int *file_id = kmalloc(sizeof(int), GFP_KERNEL);
       if (!file_id)
               return -ENOMEM;
       *file_id = counter->file_counter++;
       file->private_data = file_id;
       pr_info("demo device operation %d: open of file number %d\n", counter->operation_counter++, *file_id);
       return 0;
}

static int demo_release(struct inode *inode, struct file *file)
{
       struct device *device = class_find_device_by_devt(demo_class, inode->i_rdev);
       struct demo_operation_counter *counter = dev_get_drvdata(device);
       int *file_id = file->private_data;
       pr_info("demo device operation %d: release of file number %d\n", counter->operation_counter++, *file_id);
       return 0;
}

static ssize_t demo_read(struct file *file, char __user *buf, size_t size, loff_t *off)
{
       struct device *device = class_find_device_by_devt(demo_class, file->f_inode->i_rdev);
       struct demo_operation_counter *counter = dev_get_drvdata(device);
       int *file_id = file->private_data;
       pr_info("demo device operation %d: read of file numer %d\n", counter->operation_counter++, *file_id);
       return 0;
}

static ssize_t demo_write(struct file *file, const char __user *buf, size_t size, loff_t *off)
{
       struct device *device = class_find_device_by_devt(demo_class, file->f_inode->i_rdev);
       struct demo_operation_counter *counter = dev_get_drvdata(device);
       int *file_id = file->private_data;
       pr_info("demo device operation %d: write of file numer %d\n", counter->operation_counter++, *file_id);
       return size;
}

static long demo_unlocked_ioctl(struct file *file, unsigned int, unsigned long)
{
       struct device *device = class_find_device_by_devt(demo_class, file->f_inode->i_rdev);
       struct demo_operation_counter *counter = dev_get_drvdata(device);
       int *file_id = file->private_data;
       pr_info("demo device operation %d: ioctl of file number %d\n", counter->operation_counter++, *file_id);
       return 0;
}

static loff_t demo_llseek(struct file *file, loff_t, int)
{
       struct device *device = class_find_device_by_devt(demo_class, file->f_inode->i_rdev);
       struct demo_operation_counter *counter = dev_get_drvdata(device);
       int *file_id = file->private_data;
       pr_info("demo device operation %d: llseek of file number %d\n", counter->operation_counter++, *file_id);
       return 0;
}


const static struct file_operations demo_fops = {
       .owner = THIS_MODULE,
       .open = demo_open,
       .release = demo_release,
       .read = demo_read,
       .write = demo_write,
       .unlocked_ioctl = demo_unlocked_ioctl,
       .llseek = demo_llseek,
};

static char *demo_devnode(struct device *dev, umode_t *mode)
{
       if (mode)
               *mode = 0666;
       return NULL;
}

#define DEMO_NUM_MINORS 1

int __init demo_init(void)
{
       int ret;
       struct demo_operation_counter *counter;
       ret = alloc_chrdev_region(&device_number, 0, DEMO_NUM_MINORS, "demo");
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

       cdev_init(cdev, &demo_fops);
       ret = cdev_add(cdev, device_number, DEMO_NUM_MINORS);
       if (ret) {
               pr_err("Unable to add cdev %i\n", ret);
               goto err_cdev_add;
       }
       demo_class = class_create(THIS_MODULE, "demo");
       if (IS_ERR(demo_class)) {
               ret = PTR_ERR(demo_class);
               pr_err("Unable to create class for demo devices: %i\n", ret);
               goto err_cdev_add;
       }
       demo_class->devnode = demo_devnode;
       counter = kzalloc(sizeof(*counter), GFP_KERNEL);
       if (!counter) {
               ret = -ENOMEM;
               pr_err("Unable to create counter for demo devices: %i\n", ret);
               goto err_class_create;
       }
       demo_device = device_create(demo_class, NULL, device_number, counter, "demo");
       if (IS_ERR(demo_device)) {
               ret = PTR_ERR(demo_device);
               pr_err("Unable to create device for demo devices: %i\n", ret);
               goto err_counter_allocate;
       }
       pr_info("demo device operation %d: init\n", counter->operation_counter++);
       return 0;


err_counter_allocate:
               kfree(counter);
err_class_create:
               class_destroy(demo_class);
err_cdev_add:
               cdev_del(cdev);
err_cdev_alloc:
               unregister_chrdev_region(device_number, DEMO_NUM_MINORS);
err_alloc:
               return ret;
}

void __exit demo_exit(void)
{
       struct device *device = class_find_device_by_devt(demo_class, device_number);
       struct demo_operation_counter *counter = dev_get_drvdata(device);
       pr_info("demo device operation %d: exit\n", counter->operation_counter++);
       kfree(counter);
       device_destroy(demo_class, device_number);
       class_destroy(demo_class);
       cdev_del(cdev);
       unregister_chrdev_region(device_number, DEMO_NUM_MINORS);
}

module_init(demo_init);
module_exit(demo_exit);