/*

Linux 的字符设备驱动，需要实现设备文件的open、close、read函数，也就是引用Linux下的
inlcude/fs.h 中file_operations结构体并实现其(部分)成员变量

驱动属于内核级的东西，用户程序属于用户级，所以驱动程序的编写相当于给应用程序搭建到
内核 -> 寄存器的操作桥梁。所以驱动程序相当于是一个必须有的库函数，通过文件的形式暴露
操控接口给用户

由于linux的结构，驱动程序有2种加载方式，1是随内核一起编译为镜像，2是通过insmod等函数，
以模块 .ko的形式加载到系统。一般用模块方式。

Linux加载驱动 insmod，卸载驱动rmmod，所以在驱动代码中就需要注册这两种方法的实现函数
使用module_init( init_func );和 module_exit( exit_func );定义。

使用static修饰：延长局部变量的生命周期，全局变量和函数只能在本文件中调用，避免冲突。

__init /__initdata修饰符用于表示指定内容（函数 / 变量）放在可执行文件的__init节区中，
该节区只能在初始化阶段时使用，用后会被释放。exit同理。

*/
#include <linux/module.h> //module_init/exit
#include <linux/init.h>   //MODULE_LICENCE...
#include <linux/fs.h>      //register_chrdev
#include <linux/kernel.h>   //printk
#include <linux/uaccess.h>  //copy_from/to_user

#define DEV_MAJOR 114
#define DEV_NAME "chrdevbase"

static char read_buf[100] = {0};
static char write_buf[100] = {0};
static char kerneldata[] = {"kernel data Out!"};

static int chrdev_open(struct inode *inode, struct file *file)
{
    return 0;   //0成功，其他失败
}
static int chrdev_release(struct inode *inode, struct file *file)
{
    return 0;   //0成功，其他失败
}

static ssize_t chrdev_read(struct file *file, char __user *buf,
			  size_t cnt, loff_t *ppos)
{
    int ret = 0;
    memcpy(read_buf,kerneldata,sizeof(kerneldata));
    ret = copy_to_user(buf,read_buf,cnt);
    if(ret == 0)
        printk("chrdev_read ok\n");
    else
        printk("chrdev_read failed\n");
    return 0;
}

static ssize_t chrdev_write(struct file *file, const char __user *buf,
			  size_t cnt, loff_t *ppos)
{
    int ret = 0;
    ret = copy_from_user(write_buf,buf,cnt);
    if(ret == 0)
        printk("chrdev recieve %s\n",write_buf);
    else
        printk("chrdev recieve failed\n");
    return 0;
}

static const struct file_operations chrdev_fops = {
    .owner = THIS_MODULE,
    .open = chrdev_open,
    .release = chrdev_release,
    .read = chrdev_read,
    .write = chrdev_write,
};
static int __init  chrbase_init(void)
{
    /* 注册字符设备 */
    int ret = register_chrdev(DEV_MAJOR,DEV_NAME,&chrdev_fops);
    if (ret < 0)
    {
        printk("chrdev_register error\n");
    }
    printk("register %s succese!\n",DEV_NAME);
    return 0;
}

static void __exit chrbase_fini(void)
{
    /* 注销字符设备 */
    unregister_chrdev(DEV_MAJOR,DEV_NAME);
	printk("exit %s succese!\n",DEV_NAME);
}

module_init(chrbase_init);
module_exit(chrbase_fini);

MODULE_LICENSE("GPL2");
MODULE_AUTHOR("MY");
MODULE_DESCRIPTION(DEV_NAME);
MODULE_ALIAS("test module");