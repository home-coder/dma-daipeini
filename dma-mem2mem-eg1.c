/***************************************************************************
 > Filename   : dma.c
 > Author     : oneface - one_face@sina.com
 > Company    : 一尊还酹江月
 > Time       : 2018-06-14 10:24:50
 > Description: 

 - This program is free software; you can redistribute it and/or modify it
 - under the terms of the GNU General Public License as published by the
 - Free Software Foundation;  either version 2 of the  License, or (at your
 - option) any later version.
 **************************************************************************/
#include <linux/module.h>
#include <linux/kernel.h>	/* printk() */
#include <linux/moduleparam.h>
#include <asm/uaccess.h>
#include <asm/pgtable.h>
#include <linux/fs.h>
#include <linux/gfp.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/mm.h>
#include <linux/kdev_t.h>
#include <linux/proc_fs.h>
#include <linux/ioctl.h>
#include <linux/slab.h>
#include <linux/mempool.h>
#include <linux/mm.h>
#include <linux/device.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <asm/types.h>
#include <asm/io.h>
#include <asm/dma-mapping.h>

#define DEST_ADDRESS 0x73800000
#define SRC_ADDRESS   0x10400000

static struct completion comp1;
static struct dma_chan *dma_chan1;

static int device_mmap(struct file *file, struct vm_area_struct *vma)
{
	int size;
	printk("mmap memdma\n");
	size = vma->vm_end - vma->vm_start;
	if (remap_pfn_range(vma, vma->vm_start, DEST_ADDRESS >> PAGE_SHIFT, 0x800000, vma->vm_page_prot))
		return -EAGAIN;
	return 0;
}

static void dma_complete_func(void *completion)
{
	complete(completion);
}

struct dma_async_tx_descriptor *tx1 = NULL;

static long device_ioctl(struct file *file, unsigned int cmd, unsigned long param)
{
	struct dma_device *dma_dev;
	enum dma_ctrl_flags flags;
	dma_cookie_t cookie;
	dma_dev = dma_chan1->device;
	flags = DMA_CTRL_ACK | DMA_COMPL_SKIP_DEST_UNMAP | DMA_COMPL_SKIP_SRC_UNMAP;
	tx1 = dma_dev->device_prep_dma_memcpy(dma_chan1, DEST_ADDRESS, SRC_ADDRESS + 0x40000 * param, 0x40000, flags);
	if (!tx1) {
		printk("Failed to prepare DMA memcpy\n");
		return -1;
	}
	init_completion(&comp1);
	tx1->callback = dma_complete_func;
	tx1->callback_param = &comp1;

	cookie = tx1->tx_submit(tx1);
	if (dma_submit_error(cookie)) {
		printk("Failed to do DMA tx_submit\n");
		return -1;
	}

	dma_async_issue_pending(dma_chan1);

	wait_for_completion(&comp1);

	return 0;
}

static struct file_operations memdma_fops = {
	.owner = THIS_MODULE,
	.mmap = device_mmap,
	.unlocked_ioctl = device_ioctl,
};

static int __init hello_init(void)
{
	//dev_t memdma_dev;
	//struct device *dev;
	//dma_addr_t dm;
	struct cdev *memdma_cdev;
	dma_cap_mask_t mask;

	dma_cap_zero(mask);
	dma_cap_set(DMA_MEMCPY, mask);
	dma_chan1 = dma_request_channel(mask, 0, NULL);

	if (dma_chan1 == 0) {
		printk("memdma:failed to request DMA channel\n");
	}

	if (register_chrdev_region(MKDEV(200, 0), 1, "memdma")) {
		printk(KERN_INFO "alloc chrdev error.\n");
		return -1;
	}

	memdma_cdev = cdev_alloc();
	if (!memdma_cdev) {
		printk(KERN_INFO "cdev alloc error.\n");
		return -1;
	}
	memdma_cdev->ops = &memdma_fops;
	memdma_cdev->owner = THIS_MODULE;

	if (cdev_add(memdma_cdev, MKDEV(200, 0), 1)) {
		printk(KERN_INFO "cdev add error.\n");
		return -1;
	}
	printk("memdma driver loaded\n");

	return 0;

}

late_initcall(hello_init);
MODULE_LICENSE("GPL");
