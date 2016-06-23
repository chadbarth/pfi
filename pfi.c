/* Copyright (c) Vorne Industries */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/interrupt.h> 
#include <linux/phy.h>
#include <linux/netdevice.h>
#include <linux/kobject.h>

#define DRV_NAME "pfi"
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vorne Industries");
MODULE_DESCRIPTION("Take emergency actions on power fail interrupt");
MODULE_VERSION("0.1");

/* A mutex will ensure that only one process accesses our device */
static DEFINE_MUTEX(pfi_mutex);

static u16 irq_number;

static ushort pin = 47;
module_param(pin, ushort, 0644);
MODULE_PARM_DESC(pin, "GPIO pin to monitor for Power Fail Interrupt.");

static char iface[IFNAMSIZ] = "eth0";
module_param_string(interface, iface, IFNAMSIZ, 0644);
MODULE_PARM_DESC(interface, "Interface to disable when interrupt was received.");

static struct class *pfi_class = NULL;
static struct device *pfi_device = NULL;

struct kernfs_node *value_sd;

/**
 * PFI handler
 * @param  irq    Interrupt number
 * @param  dev_id Device ID pointer passed in to request_irq
 * @param  regs   Snapshot of the processor's context before entering interrupt code
 */
static irq_handler_t pfi_irq_handler(unsigned int irq, void *arg, struct pt_regs *regs)
{
    struct net_device* device;

    printk(KERN_INFO DRV_NAME ": interrupt received\n");
    device = dev_get_by_name(&init_net, iface);
    device->phydev->drv->suspend(device->phydev);

    sysfs_notify_dirent(value_sd);

    return (irq_handler_t) IRQ_HANDLED;
}

static ssize_t value_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    ssize_t status;
    mutex_lock(&pfi_mutex);

    status = snprintf(buf, PAGE_SIZE, "%d\n", gpio_get_value(pin));

    mutex_unlock(&pfi_mutex);
    return status;
}

DEVICE_ATTR(value, 0444, value_show, NULL);

static void cleanup_sysfs(void)
{
    device_remove_file(pfi_device, &dev_attr_value);
    device_destroy(pfi_class, MKDEV(0, 0));
    class_destroy(pfi_class);
}
 
/** @brief The LKM initialization function
 *  @return returns 0 if successful
 */
static int __init pfi_init(void)
{
    int result;

    printk(KERN_INFO DRV_NAME ": interface=%s, pin=%d\n", iface, pin);

    pfi_class = class_create(THIS_MODULE, DRV_NAME);
    if (IS_ERR(pfi_class))
    {
        printk(KERN_ALERT DRV_NAME ": failed to register device class '%s'\n", DRV_NAME);
        result = PTR_ERR(pfi_class);
        return result;
    }

    pfi_device = device_create(pfi_class, NULL, MKDEV(0, 0), NULL, DRV_NAME "0");
    if (IS_ERR(pfi_device))
    {
        printk(KERN_ALERT DRV_NAME ": failed to create device '%s'\n", DRV_NAME "0");
        result = PTR_ERR(pfi_device);
        class_destroy(pfi_class);
        return result;
    }

    result = device_create_file(pfi_device, &dev_attr_value);
    if (result < 0)
    {
        printk(KERN_INFO DRV_NAME ": failed to create /sys endpoint\n");
        device_destroy(pfi_class, MKDEV(0, 0));
        class_destroy(pfi_class);
        return result;
    }

    value_sd = sysfs_get_dirent(pfi_device->kobj.sd, "value");
    if(value_sd == NULL)
    {
        printk(KERN_ALERT DRV_NAME ": failed to get file descriptor\n");
        cleanup_sysfs();
        return -1;
    } 

    result = gpio_request_one(pin, GPIOF_IN, DRV_NAME " irq");
    if (result >= 0)
    {
        irq_number = gpio_to_irq(pin);

        if (irq_number >= 0)
        {
            result = request_threaded_irq(irq_number, NULL, (irq_handler_t) pfi_irq_handler,
                                             IRQF_ONESHOT | IRQF_TRIGGER_FALLING, DRV_NAME "_irq_handler", value_sd);
        }
        else
        {
            printk(KERN_ALERT DRV_NAME ": failed to get IRQ for pin %d.\n", pin);
            result = irq_number;
            gpio_free(pin);
            cleanup_sysfs();
        }
    }
    else
    {
        printk(KERN_ALERT DRV_NAME ": failed to request IRQ pin %d.\n", pin);
        cleanup_sysfs();
    }

    printk(KERN_INFO DRV_NAME ": initialization %s\n", result < 0 ? "failed" : "succeeded");
    return result;
}

/** @brief The LKM cleanup function
 */
static void __exit pfi_exit(void)
{
    free_irq(irq_number, value_sd);
    
    gpio_free(pin);
    
    cleanup_sysfs();

    printk(KERN_INFO DRV_NAME ": unloaded\n");
}
 
module_init(pfi_init);
module_exit(pfi_exit);
