#include <linux/module.h> //module init/exit
#include <linux/init.h>   //MODULE_data
#include <linux/fs.h>     //file_operations alloc_chrdev_region
#include <linux/kernel.h> //printk
#include <linux/uaccess.h>//copy_from/to_user
#include <linux/io.h>     //IO read/write
#include <linux/cdev.h>
#include <linux/device.h>

#define DEV_NAME "my_led"
#define DEV_COUNT 3

struct led_dev{
    dev_t dev_id;
    int devid_major,devid_minor;
    struct cdev cdev;
    unsigned int __iomem *va_dr;
    unsigned int __iomem *va_gdir;
    unsigned int __iomem *va_iomuxc_mux;
    unsigned int __iomem *va_ccm_ccgrx;
    unsigned int __iomem *va_iomux_pad;

    unsigned long pa_dr;
    unsigned long pa_gdir;
    unsigned long pa_iomuxc_mux;
    unsigned long pa_ccm_ccgrx;
    unsigned long pa_iomux_pad;

    unsigned int led_pin;
    unsigned int clock_offset;
};
static struct led_dev leds[DEV_COUNT] = {
    {.pa_dr = 0x0209C000,.pa_gdir = 0x0209C004,.pa_iomuxc_mux =
    0x20E006C,.pa_ccm_ccgrx = 0x20C406C,.pa_iomux_pad =
    0x20E02F8,.led_pin = 4,.clock_offset = 26},
    {.pa_dr = 0x20A8000,.pa_gdir = 0x20A8004,.pa_iomuxc_mux =
    0x20E01E0,.pa_ccm_ccgrx = 0x20C4074,.pa_iomux_pad =
    0x20E046C,.led_pin = 20,.clock_offset = 12},
    {.pa_dr = 0x20A8000,.pa_gdir = 0x20A8004,.pa_iomuxc_mux =
    0x20E01DC,.pa_ccm_ccgrx = 0x20C4074,.pa_iomux_pad =
    0x20E0468,.led_pin = 19,.clock_offset = 12},
};

static struct class *class;
static dev_t dev_id = 0;

/* led file_operations */
static int led_file_open(struct inode *inode, struct file *filp)
{
    unsigned int val = 0;
    struct led_dev *led_cdev =(struct led_dev *)container_of(inode->i_cdev, struct led_dev,cdev);
    //inode->i_cdev记录cdev的地址，此地址又包含在led_dev结构体内，通过container_of来找到led_dev结构体的首地址
    //cdev不是第一个结构体成员也没事，container_of会自动计算出成员地址，反推出结构体地址
    filp->private_data = led_cdev;
    //3个LED都分别为不同的cdev和不同的device文件，通过device文件绑定的inode->i_cdev得到led的cdev结构地址
    //就能得到该LED的操控结构体

    printk("open\n");
    /* 实现地址映射 */
    led_cdev->va_dr = ioremap(led_cdev->pa_dr, 4);  //,数据寄存器映射，将led_cdev->va_dr指针指向映射后的虚拟地址起始处，这段地址大小为4个字节
    led_cdev->va_gdir = ioremap(led_cdev->pa_gdir, 4);      //方向寄存器映射
    led_cdev->va_iomuxc_mux = ioremap(led_cdev->pa_iomuxc_mux, 4);  //端口复用功能寄存器映射
    led_cdev->va_ccm_ccgrx = ioremap(led_cdev->pa_ccm_ccgrx, 4);    //时钟控制寄存器映射
    led_cdev->va_iomux_pad = ioremap(led_cdev->pa_iomux_pad, 4);    //电气属性配置寄存器映射
    /* 配置寄存器 */
    val = ioread32(led_cdev->va_ccm_ccgrx); //间接读取寄存器中的数据
    val &= ~(3 << led_cdev->clock_offset);
    val |= (3 << led_cdev->clock_offset);   //置位对应的时钟位
    iowrite32(val, led_cdev->va_ccm_ccgrx); //重新将数据写入寄存器
    iowrite32(5, led_cdev->va_iomuxc_mux);  //复用位普通I/O口
    iowrite32(0x1F838, led_cdev->va_iomux_pad);

    val = ioread32(led_cdev->va_gdir);
    val &= ~(1 << led_cdev->led_pin);
    val |= (1 << led_cdev->led_pin);
    iowrite32(val, led_cdev->va_gdir);      //配置位输出模式

    val = ioread32(led_cdev->va_dr);
    val |= (0x01 << led_cdev->led_pin);
    iowrite32(val, led_cdev->va_dr);        //输出高电平

    printk("led_file_open\n");
    return 0;
}

static int led_file_release(struct inode *inode, struct file *file)
{
    struct led_dev *dev = file->private_data;//其他带有struct file的可以直接这样使用
    //由于虚拟内存使用open进行分配，则需要在release函数释放，否则会无穷申请
    //默认关灯
    unsigned long val;
    val = ioread32(dev->va_dr);
    val |= (0x01 << dev->led_pin);
    iowrite32(val, dev->va_dr);
    iounmap(dev->va_dr);
    iounmap(dev->va_gdir);
    iounmap(dev->va_iomuxc_mux);
    iounmap(dev->va_ccm_ccgrx);
    iounmap(dev->va_iomux_pad);
    printk("led_file_release\n");
    return 0;
}

static ssize_t led_file_read(struct file *file, char __user *buf,size_t cnt, loff_t *ppos)
{
    printk("led_file_read\n");
    return 0;
}

static ssize_t led_file_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *ppos)
{
    
    unsigned long val = 0;
    unsigned long ret = 0;
    int tmp = cnt;
    kstrtoul_from_user(buf, tmp, 10, &ret); //kernel str to unsigned long from user
    //以10进制形式，转换buf字符串，转换长度为tmp长，返回值存入ret
    struct led_dev *led_cdev = (struct led_dev *)filp->private_data;
    val = ioread32(led_cdev->va_dr);
    if (ret == 1)   //亮灯
            val &= ~(0x01 << led_cdev->led_pin);
    else
            val |= (0x01 << led_cdev->led_pin);
    iowrite32(val, led_cdev->va_dr);
    *ppos += tmp;
    return tmp;
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

    // //申请虚拟内存
    // ledr_vaddr[ccm_clock] = ioremap(CCM_CCGR1,4);
    // ledr_vaddr[mux] = ioremap(IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO04,4);
    // ledr_vaddr[pad] = ioremap(IOMUXC_SW_PAD_CTL_PAD_GPIO1_IO04,4);
    // ledr_vaddr[dir] = ioremap(GPIO1_GDIR,4);
    // ledr_vaddr[dr] = ioremap(GPIO1_DR,4);

    // unsigned int val = 0;
    // val = ioread32(ledr_vaddr[ccm_clock]);
    // val &=~( 3 << 26);
    // val |= 3 << 26;
    // iowrite32(val,ledr_vaddr[ccm_clock]);   //开启GPIO1时钟
    // iowrite32(5,ledr_vaddr[mux]);           //配置复用功能
    // iowrite32(0x1F838,ledr_vaddr[pad]);     //配置引脚属性0x10B0
    // val = readl(ledr_vaddr[dir]);
    // val &= ~(1 << 4); 
    // val |= (1 << 4); 
    // iowrite32(val,ledr_vaddr[dir]);         //配置推挽输出
    // val = readl(ledr_vaddr[dr]);
    // val |= (1 << 4);
    // iowrite32(val, ledr_vaddr[dr]);         //配置默认关闭

    //使用alloc自动申请id号
    int i = 0;
    alloc_chrdev_region(&dev_id,0,DEV_COUNT,DEV_NAME);
    if(dev_id <= 0){
        printk("alloc_chrdev_region\n");
        return -1;
    }
    class = class_create(THIS_MODULE,DEV_NAME); //创建设备类
    if(IS_ERR(class)){
        return PTR_ERR(class);
    }
    dev_t cur_dev = 0;
    for(i = 0;i < DEV_COUNT;i++)        //创建多个字符设备及其device结构
    {
        cdev_init(&leds[i].cdev,&led_fops);
        leds[i].cdev.owner = THIS_MODULE;
        cur_dev = MKDEV(MAJOR(dev_id),MINOR(dev_id)+i);
        leds[i].dev_id = cur_dev;
        cdev_add(&leds[i].cdev,cur_dev,1);
        device_create(class,NULL,cur_dev,NULL,DEV_NAME"%d",i);//device注销时只需要class和设备，所以可以不使用device变量存储
    }
    return 0;
}

static void __exit led_module_exit(void)
{
    int i = 0;
    for(i = 0;i < DEV_COUNT;i++)
    {
        device_destroy(class,leds[i].dev_id);
        cdev_del(&leds[i].cdev);
    }
    class_destroy(class);
    unregister_chrdev_region(dev_id,DEV_COUNT);
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