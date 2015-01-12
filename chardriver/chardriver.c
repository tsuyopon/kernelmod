#include <linux/module.h>       /* MODULE macro, THIS_MODULE */
#include <linux/moduleparam.h>
#include <linux/kernel.h>       /* printk() */
#include <linux/fs.h>           /* alloc_chrdev_region(), ... */
#include <linux/types.h>        /* dev_t */
#include <linux/kdev_t.h>       /* MAJOR() */
#include <linux/stat.h>         /* S_IRUGO, S_IWUSR */

MODULE_AUTHOR("Azarashi");
MODULE_LICENSE("GPL");

#define SIMPLE_CHAR_BUFSIZE 10
#define SIMPLE_CHAR_NR_DEVS 4 /* マイナーデバイス番号の総数 */
#define SIMPLE_CHAR_MAJOR 0   /* 0の場合にはデバイス番号を動的に割り当てる */
#define SIMPLE_CHAR_DEVNAME "mychar"

static int simple_char_bufsize = SIMPLE_CHAR_BUFSIZE;
static int simple_char_nr_devs = SIMPLE_CHAR_NR_DEVS;
static int simple_char_major   = SIMPLE_CHAR_MAJOR;
static int simple_char_minor   = 0;

// メジャー番号: カーネルがドライバを識別するための番号
// マイナー番号: ドライバがデバイスを識別するための番号

static int simple_char_setup_devnr(void)
{
        int result;
        dev_t dev;

        if(simple_char_major) {
                /* ユーザ指定のデバイス番号を登録する*/
                dev = MKDEV(simple_char_major, simple_char_minor);
                result = register_chrdev_region(dev, simple_char_nr_devs, SIMPLE_CHAR_DEVNAME);  // 指定したメジャー番号を登録する
        } else {
                /* デバイス番号を動的に確保する */
                result = alloc_chrdev_region(&dev, simple_char_minor, simple_char_nr_devs, SIMPLE_CHAR_DEVNAME); // 他のデバイスとデバイス番号が被らないように、メジャー番号を動的に確保する
                simple_char_major = MAJOR(dev);  // メジャー番号を取得する
        }
        if(result < 0) {
                printk(KERN_WARNING "chardriver: fail to get major %d\n", simple_char_major);
                return result;
        }

        return 0;
}

static int simple_char_clear_devnr(void)
{
        dev_t dev = MKDEV(simple_char_major, simple_char_minor);
        unregister_chrdev_region(dev, simple_char_nr_devs);       // 登録したデバイス番号は、モジュールのアンロード時に解放が必要
        return 0;
}


static int simple_char_init(void)
{
        int result;

        printk(KERN_ALERT "simple_char: %s\n", __FUNCTION__);
        printk(KERN_ALERT "  simple_char_bufsize = %d\n", simple_char_bufsize);

        result = simple_char_setup_devnr();
        if(result) {
                goto fail;
        }

        return 0;

fail:
        printk(KERN_WARNING "simple_char: init fail\n");
        return result;
}

static void simple_char_exit(void)
{
        simple_char_clear_devnr();
        return ;
}

module_param(simple_char_bufsize, int, S_IRUGO| S_IWUSR);
module_param(simple_char_nr_devs, int, S_IRUGO| S_IWUSR);
module_param(simple_char_major, int, S_IRUGO| S_IWUSR);
module_init(simple_char_init);
module_exit(simple_char_exit);

