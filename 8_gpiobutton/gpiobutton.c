#include <linux/module.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/cdev.h>

#define DEVCNT 1
#define DEVNAME "gpiobutton"

struct button_dev{
    dev_t devid;
    int major;
    int minor;
    struct cdev cdev;
};
static struct button_dev button;
static struct class *button_class;

int button_open(struct inode *inode, struct file *file);
int button_release(struct inode *inode, struct file *file);
ssize_t button_read(struct file *file, char __user *buffer, size_t cnt, loff_t *pos);
static const struct file_operations button_fops = {
    .owner = THIS_MODULE,
    .open = button_open,
    .release = button_release,
    .read = button_read,
};
static int __init button_init(void)
{
    int ret = 0;

    /* 1. 申请设备号 */
    button.major = 0;
    if(button.major != 0){
        button.devid = MKDEV(button.major,button.minor);
        ret = register_chrdev_region(button.devid,DEVCNT,DEVNAME);
    }    
    else
        ret = alloc_chrdev_region(&button.devid,0,DEVCNT,DEVNAME);
    if(ret < 0){ printk(KERN_ERR "alloc devid error\r\n"); goto ALLOC_ERR; }
    
    /* 2. 注册cdev */
    button.cdev.owner = THIS_MODULE;
    cdev_init(&button.cdev,&button_fops);
    ret = cdev_add(&button.cdev,button.devid,1);
    if(ret < 0){ printk(KERN_ERR "cdev add error\r\n"); goto CDEV_ERR; }

    /* 3. 注册class */
    button_class = class_create(THIS_MODULE,DEVNAME);
    if(IS_ERR(button_class)){
        printk(KERN_ERR "class create failed\r\n");
        ret = -EINVAL;
        goto CLASS_ERR;
    }

    /* 4. 注册设备 */
    if(IS_ERR(device_create(button_class,NULL,button.devid,NULL,DEVNAME))){
        printk(KERN_ERR "device create failed\r\n");
        ret = -EINVAL;
        goto DEVICE_ERR;
    }

    return 0;

DEVICE_ERR:
    class_destroy(button_class);
CLASS_ERR:
    cdev_del(&button.cdev);
CDEV_ERR:
    unregister_chrdev_region(button.devid,DEVCNT);
ALLOC_ERR:
    return ret;
}

static void __exit button_exit(void)
{
    cdev_del(&button.cdev);
    unregister_chrdev_region(button.devid,DEVCNT);
    device_destroy(button_class,button.devid);
    class_destroy(button_class);
    printk("button module exit\r\n");
}

module_init(button_init);
module_exit(button_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("my");

/* fops */
int button_open(struct inode *inode, struct file *file)
{
    file->private_data = &button;
    return 0;
}
int button_release(struct inode *inode, struct file *file)
{
    file->private_data = NULL;
    return 0;
}
ssize_t button_read(struct file *file, char __user *buffer, size_t cnt, loff_t *pos)
{
    struct button_dev *button = file->private_data;
    return 0;
}