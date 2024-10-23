#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("MIPT-license");
MODULE_VERSION("0.0.1");
int hello_init(void);
void hello_cleanup(void);


int hello_init(void) {
	printk("Hello world!");

	return 0;
}

void hello_cleanup(void) {
	printk("Goodbye world");
}

module_init(hello_init);
module_exit(hello_cleanup);
