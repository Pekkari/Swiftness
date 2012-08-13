
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

static int swt_init(void)
{

}

static void swt_exit(void)
{

}

module_init(swt_init);
module_exit(swt_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("José Ramón Muñoz Pekkarinen");
MODULE_DESCRIPTION("Openstack Swift support for NBD module");
