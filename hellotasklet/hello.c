#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/sched.h>

MODULE_AUTHOR("Azarashi");
MODULE_DESCRIPTION("test_tasklet");
MODULE_LICENSE("GPL");

static unsigned long testlong;
char testdata[] = "hello,tasklet\n";

void tasklet_callback(unsigned long data)
{
	struct task_struct *p = current;
	pr_info("%c [%d] %s cpu:%d %s",
		in_interrupt() ? 'Y' : 'N',
		p->pid,
		p->comm,
		smp_processor_id(),
		(char *)data);
}

DECLARE_TASKLET(my_tasklet, tasklet_callback,
		(unsigned long)&testdata);

static irqreturn_t my_irq_handler(int irq, void *dev_id)
{
	tasklet_schedule(&my_tasklet);
	return IRQ_HANDLED;
}

static int __init my_tasklet_init(void)
{
	int ret = 0;

	// IRQF_SHAREDは1つのIRQに複数のハンドラを登録することができる。
	// I/F参考: http://wiki.onakasuita.org/pukiwiki/?request_irq
	//
	// irq –      Interrupt line to allocate.
	// handler –  Function to be called when the IRQ occurs.
	// irqflags – Interrupt type flags.
	// devname –  device name, this is shown in the /proc/interrupts – to show the owner of an interrupt.
	// dev_id –  a reference passed to the handler function. It shouldn’t be kept null, and it useful while freeing up the interrupt line.
	ret = request_irq(1, my_irq_handler, IRQF_SHARED, "test_tasklet", (void *)&testlong);
	if (ret)
		pr_err("failed request_irq: %d", ret);

	return ret;
}

static void __exit my_tasklet_exit(void)
{
	free_irq(1, (void *)&testlong);
	tasklet_kill(&my_tasklet);
}

module_init(my_tasklet_init);
module_exit(my_tasklet_exit);

