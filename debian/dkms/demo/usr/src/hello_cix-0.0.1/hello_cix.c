#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>

static int __init hello_cix_init(void)
{
    printk("Hello CIX!\n");
    return 0;
}

static void __exit hello_cix_exit(void)
{
    printk("Goodbye CIX!\n");
}

module_init(hello_cix_init);
module_exit(hello_cix_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("CIX");
MODULE_DESCRIPTION("A simple demo driver to verify dkms");