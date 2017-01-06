/*
 * タスクレットのサンプル
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>

MODULE_LICENSE("GPL");

void hoge_fun(unsigned long data) {
	printk("%s\n", (char *)data);
}

// タスクレットを宣言する
DECLARE_TASKLET(hoge_tasklet, hoge_fun, 0);

void invoke_virtual_irq(void) {
       hoge_tasklet.data = (unsigned long)"hoge by tasklet";
       tasklet_schedule(&hoge_tasklet);
}

// モジュールの初期化はinit_moduleという関数名と決まっている。
// 自分で名前をつけたい場合は、module_init(<func_name>)とする
int init_module(void) {
       invoke_virtual_irq();
       printk("after tasklet_schedule\n");
       return 0;
}

// モジュールの解放処理はcleanup_moduleという関数名と決まっている。
// 自分で名前をつけたい場合は、module_exit(<func_name>)とする
void cleanup_module(void) {
       tasklet_kill(&hoge_tasklet);
}

// module_initとmodule_exitはinit_module, cleanup_moduleから関数名が変わる時**だけ**定義する。(でないとエラーになる)
//module_init(<funcname>);
//module_exit(<funcname>);
