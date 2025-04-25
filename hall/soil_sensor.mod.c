#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xcc3f7cf8, "gpio_to_desc" },
	{ 0xced6392c, "gpiod_get_raw_value" },
	{ 0x656e4a6e, "snprintf" },
	{ 0x122c3a7e, "_printk" },
	{ 0x15ba50a6, "jiffies" },
	{ 0xc38c83b8, "mod_timer" },
	{ 0x47229b5c, "gpio_request" },
	{ 0xfb30e719, "gpiod_direction_input" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0x282c842f, "cdev_init" },
	{ 0xe72ec58e, "cdev_add" },
	{ 0x1b58158e, "class_create" },
	{ 0xcc4889d4, "device_create" },
	{ 0xc6f46339, "init_timer_key" },
	{ 0xfe990052, "gpio_free" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x9875d727, "cdev_del" },
	{ 0x82ee90dc, "timer_delete_sync" },
	{ 0xe3d97303, "device_destroy" },
	{ 0xf4e1b5d, "class_destroy" },
	{ 0x98cf60b3, "strlen" },
	{ 0x6cbbfc54, "__arch_copy_to_user" },
	{ 0x47e64c59, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "9C238B7B6F00DF55B86C610");
