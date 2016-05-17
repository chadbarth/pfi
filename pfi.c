/* Copyright (c) Vorne Industries */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/interrupt.h> 
#include <linux/phy.h>
#include <linux/netdevice.h>

#define DRV_NAME "pfi"
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vorne Industries");
MODULE_DESCRIPTION("Take emergency actions on power fail interrupt");
MODULE_VERSION("0.1");

static u16 irq_number;

static ushort pin = 47;
module_param(pin, ushort, 0644);
MODULE_PARM_DESC(pin, "GPIO pin to monitor for Power Fail Interrupt.");

static char iface[IFNAMSIZ] = "eth0";
module_param_string(interface, iface, IFNAMSIZ, 0644);
MODULE_PARM_DESC(interface, "Interface to disable when interrupt was received.");

/**
 * PFI handler
 * @param  irq    Interrupt number
 * @param  dev_id Device ID pointer passed in to request_irq
 * @param  regs   Snapshot of the processor's context before entering interrupt code
 */
static irq_handler_t pfi_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs) {
    struct net_device* device;

    printk(KERN_INFO DRV_NAME ": interrupt received\n");
    device = dev_get_by_name(&init_net, iface);
    device->phydev->drv->suspend(device->phydev);

    return (irq_handler_t) IRQ_HANDLED;
}

 
/** @brief The LKM initialization function
 *  @return returns 0 if successful
 */
static int __init pfi_init(void){

    int result = gpio_request_one(pin, GPIOF_IN, DRV_NAME " irq");

    printk(KERN_INFO DRV_NAME ": interface=%s, pin=%d\n", iface, pin);

    if (result >= 0)
    {
        irq_number = gpio_to_irq(pin);
        if (irq_number >= 0)
        {
            result = request_threaded_irq(irq_number, NULL, (irq_handler_t) pfi_irq_handler,
                                             IRQF_ONESHOT | IRQF_TRIGGER_FALLING, DRV_NAME "_irq_handler", NULL);
        }
        else
        {
            printk(KERN_ALERT DRV_NAME ": failed to get IRQ for pin %d.\n", pin);
            result = irq_number;
            gpio_free(pin);
        }
    }
    else
    {
        printk(KERN_ALERT DRV_NAME ": failed to request IRQ pin %d.\n", pin);
    }


    printk(KERN_INFO DRV_NAME ": initialization %s\n", result < 0 ? "failed" : "succeeded");
    return result;
}
 
/** @brief The LKM cleanup function
 */
static void __exit pfi_exit(void){
   free_irq(irq_number, NULL);
    
   gpio_free(pin);
    
   printk(KERN_INFO DRV_NAME ": unloaded\n");
}
 
module_init(pfi_init);
module_exit(pfi_exit);
