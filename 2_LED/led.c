#include <linux/module.h> //module init/exit
#include <linux/init.h>   //MODULE_data
#include <linux/fs.h>     //file_operations
#include <linux/kernel.h> //printk
#include <linux/uaccess.h>//copy_from/to_user
#include <linux/io.h>

#define DEV_NAME "my_led"
#define DEV_NUM 114

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

enum LEDIO
{
    ccm_clock,
    mux,
    pad,
    dir,
    dr
};

static unsigned int __iomem *ledr_vaddr[5];

/* led file_operations */
static int led_file_open(struct inode *inode, struct file *file)
{
    printk("led_file_open\n");
    return 0;
}

static int led_file_release(struct inode *inode, struct file *file)
{
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
    int ret = 0;
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

    ret = register_chrdev(DEV_NUM,DEV_NAME,&led_fops);
    if (ret < 0)
    {
        printk("led_module_init error\n");
        return -1;
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
    unregister_chrdev(DEV_NUM,DEV_NAME);
    printk("led_module_exit\n");
}

module_init(led_module_init);
module_exit(led_module_exit);
/*end of led module operations */

//module info
MODULE_LICENSE("GPL2");
MODULE_AUTHOR("MY");
MODULE_DESCRIPTION(DEV_NAME);
MODULE_ALIAS("My LED module");