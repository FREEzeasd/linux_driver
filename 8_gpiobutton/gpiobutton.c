#include <linux/module.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/uaccess.h>

#define DEVCNT 1
#define DEVNAME "gpiobutton"

struct button_dev{
    dev_t devid;
    int major;
    int minor;
    struct cdev cdev;
};
struct button_gpio{
    struct device_node *nd;
    int id;
    atomic_t keyvalue;
};
static struct button_dev button;
static struct button_gpio gpio;
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

static int gpio_button_init(void)
{
    gpio.nd = of_find_node_by_path("/gpiobutton");
    if(gpio.nd == NULL)
    {
        printk(KERN_ERR "device node not found\r\n");
        return -EINVAL;
    }
    gpio.id = of_get_named_gpio(gpio.nd,"button-gpio",0);
    if(gpio.id < 0)
    {
        printk(KERN_ERR "GPIO not found\r\n");
        return -EINVAL;
    }
    printk("get button GPIO: %d\r\n",gpio.id);

    gpio_request(gpio.id,"button");
    gpio_direction_input(gpio.id);
    return 0;
}   

static int __init button_init(void)
{
    int ret = 0;

    ret = gpio_button_init();
    atomic_set(&gpio.keyvalue,0);
    if(ret < 0){ return ret; }

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
    //struct button_dev *button = file->private_data;
    int ret = 0;
    if(gpio_get_value(gpio.id) == 1)
    {
        while(!(gpio_get_value(gpio.id) == 1));
        atomic_set(&gpio.keyvalue,'1');
    }
    else{
        atomic_set(&gpio.keyvalue,'0');
    }
    char value = atomic_read(&gpio.keyvalue);
    ret = copy_to_user(buffer,&value,sizeof(value));
    return ret;
}