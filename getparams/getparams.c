#include <linux/module.h>
#include <linux/kernel.h>

static char* teststr = "abcdefghijklmn";
module_param( teststr, charp, S_IRUGO );
MODULE_PARM_DESC( teststr, "test parm");

// モジュール初期化
static int __init modtest_module_init( void )
{
    printk( KERN_INFO "modtest is loaded %s\n", teststr );
    return 0;
}

//  モジュール解放
static void __exit modtest_module_exit( void )
{
    printk( KERN_INFO "modtest is removed\n" );
}

module_init( modtest_module_init );
module_exit( modtest_module_exit );

MODULE_DESCRIPTION( "modtest" );
MODULE_LICENSE( "GPL2" );
