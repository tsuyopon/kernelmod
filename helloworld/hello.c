#include <linux/module.h>
#include <linux/init.h>
// Driver development do not include stdio.h and string.h ... so that these library is used by user space.

// This program's author
MODULE_AUTHOR("Azarashi");

// Indicate License
MODULE_LICENSE("Dual BSD/GPL");

// explanation about this module
MODULE_DESCRIPTION("Dual BSD/GPL");

static int hello_init(void)
{
	printk(KERN_ALERT "driver loaded\n");
	printk(KERN_ALERT "Hello World\n");
	return 0;
}

static void hello_exit(void)
{
	printk(KERN_ALERT "driver unloaded\n");
}

// Register modules
module_init(hello_init);
module_exit(hello_exit);
