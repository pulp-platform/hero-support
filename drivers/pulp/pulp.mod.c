#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xe70872b4, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0xac4f78a7, __VMLINUX_SYMBOL_STR(cdev_del) },
	{ 0x12da5bb2, __VMLINUX_SYMBOL_STR(__kmalloc) },
	{ 0xbda45038, __VMLINUX_SYMBOL_STR(cdev_init) },
	{ 0xff178f6, __VMLINUX_SYMBOL_STR(__aeabi_idivmod) },
	{ 0xfbc74f64, __VMLINUX_SYMBOL_STR(__copy_from_user) },
	{ 0x553b6202, __VMLINUX_SYMBOL_STR(up_read) },
	{ 0x5fe0e3a5, __VMLINUX_SYMBOL_STR(mem_map) },
	{ 0x2e5810c6, __VMLINUX_SYMBOL_STR(__aeabi_unwind_cpp_pr1) },
	{ 0xc87c1f84, __VMLINUX_SYMBOL_STR(ktime_get) },
	{ 0x4e93156d, __VMLINUX_SYMBOL_STR(device_destroy) },
	{ 0x7485e15e, __VMLINUX_SYMBOL_STR(unregister_chrdev_region) },
	{ 0x20b6e8d2, __VMLINUX_SYMBOL_STR(__pv_phys_offset) },
	{ 0xeeecb604, __VMLINUX_SYMBOL_STR(down_read) },
	{ 0xe2d5255a, __VMLINUX_SYMBOL_STR(strcmp) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xef174dfe, __VMLINUX_SYMBOL_STR(kunmap) },
	{ 0xdc5bf721, __VMLINUX_SYMBOL_STR(device_create) },
	{ 0xd6b8e852, __VMLINUX_SYMBOL_STR(request_threaded_irq) },
	{ 0x8e865d3c, __VMLINUX_SYMBOL_STR(arm_delay_ops) },
	{ 0x2196324, __VMLINUX_SYMBOL_STR(__aeabi_idiv) },
	{ 0x6be80ca6, __VMLINUX_SYMBOL_STR(cdev_add) },
	{ 0xbce44e09, __VMLINUX_SYMBOL_STR(kmap) },
	{ 0xdc9d4c51, __VMLINUX_SYMBOL_STR(get_user_pages) },
	{ 0xc2165d85, __VMLINUX_SYMBOL_STR(__arm_iounmap) },
	{ 0x59d29dab, __VMLINUX_SYMBOL_STR(v7_flush_kern_dcache_area) },
	{ 0x36881998, __VMLINUX_SYMBOL_STR(__dma_request_channel) },
	{ 0x4f68e5c9, __VMLINUX_SYMBOL_STR(do_gettimeofday) },
	{ 0x8aaaea2, __VMLINUX_SYMBOL_STR(dma_release_channel) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0x31ba73c0, __VMLINUX_SYMBOL_STR(remap_pfn_range) },
	{ 0x4ac98e51, __VMLINUX_SYMBOL_STR(put_page) },
	{ 0xd6ab8729, __VMLINUX_SYMBOL_STR(class_destroy) },
	{ 0xfb961d14, __VMLINUX_SYMBOL_STR(__arm_ioremap) },
	{ 0xefd6cf06, __VMLINUX_SYMBOL_STR(__aeabi_unwind_cpp_pr0) },
	{ 0x676bbc0f, __VMLINUX_SYMBOL_STR(_set_bit) },
	{ 0xac8f37b2, __VMLINUX_SYMBOL_STR(outer_cache) },
	{ 0xbd1fe2b0, __VMLINUX_SYMBOL_STR(__class_create) },
	{ 0x29537c9e, __VMLINUX_SYMBOL_STR(alloc_chrdev_region) },
	{ 0xf20dabd8, __VMLINUX_SYMBOL_STR(free_irq) },
	{ 0xe914e41e, __VMLINUX_SYMBOL_STR(strcpy) },
	{ 0x4cdb3178, __VMLINUX_SYMBOL_STR(ns_to_timeval) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

