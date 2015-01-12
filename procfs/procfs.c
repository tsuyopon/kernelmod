#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h> // create_proc_entry
#include <asm/uaccess.h> // copy_from_user

// /proc/PROCNAME にインタフェースを作る
#define PROCNAME "driver/procfs"

#define MAXBUF 8
static char procfs_buf[MAXBUF];
static int buflen;

// 書き込み
static int proc_write( struct file *filp, const char *buf, unsigned long len, void *data )
{
    if( len >= MAXBUF ){
        printk( KERN_WARNING "proc_write len = %lu\n", len );
        return -ENOSPC;
    }

    if( copy_from_user( procfs_buf, buf, len ) ) return -EFAULT;
    procfs_buf[ len ] = '\0';
    buflen = len;

    printk( KERN_INFO "proc_write %s\n", procfs_buf );

    return len;
}


// 読み込み
static int proc_read( char *page, char **start, off_t offset, int count, int *eof, void *data )
{
    unsigned long outbyte = 0;

    if( offset > 0 ){
        *eof = 1;
        return 0;
    }

    outbyte = sprintf( page, "%s", procfs_buf );
    printk( KERN_INFO "proc_read len = %lu\n", outbyte );

    *eof = 1;
    return outbyte;
}


// モジュール初期化
static int __init procfs_module_init( void )
{
    // /proc/PROCNAME にインターフェース作成
    struct proc_dir_entry* entry;
    entry = create_proc_entry( PROCNAME, 0666, NULL );
    if( entry ){
        // proc_write関数とproc_read関数を登録
        entry->write_proc  = proc_write;
        entry->read_proc  = proc_read;
    }
    else{
        printk( KERN_ERR "create_proc_entry failed\n" );
        return -EBUSY;
    }

    printk( KERN_INFO "procfs is loaded\n" );

    return 0;
}


//  モジュール解放
static void __exit procfs_module_exit( void )
{
    // インターフェース削除
    remove_proc_entry( PROCNAME, NULL );

    printk( KERN_INFO "procfs is removed\n" );
}


module_init( procfs_module_init );
module_exit( procfs_module_exit );

MODULE_DESCRIPTION("procfs");
MODULE_LICENSE( "GPL2" );
