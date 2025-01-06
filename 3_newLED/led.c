#include <linux/module.h> //module init/exit
#include <linux/init.h>   //MODULE_data
#include <linux/fs.h>     //file_operations alloc_chrdev_region
#include <linux/kernel.h> //printk
#include <linux/uaccess.h>//copy_from/to_user
#include <linux/io.h>     //IO read/write
#include <linux/cdev.h>
#include <linux/device.h>

#define DEV_NAME "my_led"

/* 寄存器地址定义 LEDR GPIO1_4*/
//时钟控制寄存器
#define CCM_CCGR1 0x20C406C
//GPIO1_04复用功能选择寄存器
#define IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO04 0x20E006C
//PAD属性设置寄存器
#define IOMUXC_SW_PAD_CTL_PAD_GPIO1_IO04 0x20E02F8
//GPIO方向设置寄存器
#define GPIO1_GDIR 0x0209C004
//GPIO输出状态寄存器
#define GPIO1_DR 0x0209C000

enum LED_REGISTERS
{
    ccm_clock,mux,pad,dir,dr
};

struct led_dev{
    dev_t dev_id;
    int devid_major,devid_minor;
    struct cdev cdev;
    struct class *class;
    struct device *device;
};

struct led_dev ledr_dev;
static unsigned int __iomem *ledr_vaddr[5];

/* led file_operations */
static int led_file_open(struct inode *inode, struct file *file)
{
    file->private_data = &ledr_dev; //将要操控设备结构体挂载到file私有数据
    printk("led_file_open\n");
    return 0;
}

static int led_file_release(struct inode *inode, struct file *file)
{
    struct led_dev *dev = file->private_data;//其他带有struct file的可以直接这样使用
    printk("led_file_release\n");
    return 0;
}

static ssize_t led_file_read(struct file *file, char __user *buf,size_t cnt, loff_t *ppos)
{
    printk("led_file_read\n");
    return 0;
}

static ssize_t led_file_write(struct file *file, const char __user *buf, size_t cnt, loff_t *ppos)
{
    int ret = 0;
    unsigned char databuf[1];
    unsigned char ledstat;
    ret = copy_from_user(databuf,buf,cnt);
    if(ret < 0)
    {
        printk("led_file_write failed\n");
        return -1;
    }
    if( databuf[0] == '1')
    {
        ledstat = 1;
    }
    else{
        ledstat = 0;
    }
    printk("recieve char %c,ledstat value %d",databuf[0],ledstat);
    unsigned int val = 0;
    val = ioread32(ledr_vaddr[dr]);
    val &= ~( 1 << 4);
    val |= (!ledstat) << 4;
    iowrite32(val,ledr_vaddr[dr]);
    
    return 0;
}

static const struct file_operations led_fops = {
    .owner = THIS_MODULE,
    .open = led_file_open,
    .release = led_file_release,
    .write = led_file_write,
    .read = led_file_read
};

/* end of led file_operations */

/* led module operations */
static int __init led_module_init(void)
{
    printk("led_module_init\n");

    //申请虚拟内存
    ledr_vaddr[ccm_clock] = ioremap(CCM_CCGR1,4);
    ledr_vaddr[mux] = ioremap(IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO04,4);
    ledr_vaddr[pad] = ioremap(IOMUXC_SW_PAD_CTL_PAD_GPIO1_IO04,4);
    ledr_vaddr[dir] = ioremap(GPIO1_GDIR,4);
    ledr_vaddr[dr] = ioremap(GPIO1_DR,4);

    unsigned int val = 0;
    val = ioread32(ledr_vaddr[ccm_clock]);
    val &=~( 3 << 26);
    val |= 3 << 26;
    iowrite32(val,ledr_vaddr[ccm_clock]);   //开启GPIO1时钟
    iowrite32(5,ledr_vaddr[mux]);           //配置复用功能
    iowrite32(0x1F838,ledr_vaddr[pad]);     //配置引脚属性0x10B0
    val = readl(ledr_vaddr[dir]);
    val &= ~(1 << 4); 
    val |= (1 << 4); 
    iowrite32(val,ledr_vaddr[dir]);         //配置推挽输出
    val = readl(ledr_vaddr[dr]);
    val |= (1 << 4);
    iowrite32(val, ledr_vaddr[dr]);         //配置默认关闭

    //使用alloc自动申请id号
    alloc_chrdev_region(&ledr_dev.dev_id,0,1,DEV_NAME);
    ledr_dev.devid_major = MAJOR(ledr_dev.dev_id);
    ledr_dev.devid_minor = MINOR(ledr_dev.dev_id);

    //初始化并添加cdev
    ledr_dev.cdev.owner = THIS_MODULE;
    cdev_init(&ledr_dev.cdev,&led_fops);
    cdev_add(&ledr_dev.cdev,ledr_dev.dev_id,1);

    //创建设备类
    ledr_dev.class = class_create(THIS_MODULE,DEV_NAME);
    if(IS_ERR(ledr_dev.class)){
        return PTR_ERR(ledr_dev.class);
    }

    //创建设备，在dev下挂在文件
    ledr_dev.device = device_create(ledr_dev.class,NULL,ledr_dev.dev_id,NULL,DEV_NAME);
    if(IS_ERR(ledr_dev.device)){
        return PTR_ERR(ledr_dev.device);
    }

    return 0;
}

static void __exit led_module_exit(void)
{
    int i = 0;
    for(i = 0;i < 5;i++)        //释放虚拟内存
    {
        iounmap(ledr_vaddr[i]);
    }
    //unregister_chrdev(DEV_NUM,DEV_NAME);
    
    cdev_del(&ledr_dev.cdev);
    unregister_chrdev_region(ledr_dev.dev_id,1);
    device_destroy(ledr_dev.class,ledr_dev.dev_id);
    class_destroy(ledr_dev.class);
    printk("led_module_exit\n");
}

module_init(led_module_init);
module_exit(led_module_exit);
/*end of led module operations */

//module info
MODULE_LICENSE("GPL");
MODULE_AUTHOR("MY");
MODULE_DESCRIPTION(DEV_NAME);
MODULE_ALIAS("My LED module");