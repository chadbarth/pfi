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
static u16 irq_pin = 47;
static u16 gpio_pin = 46;
static u16 irq_number;

/**
 * PFI handler
 * @param  irq    Interrupt number
 * @param  dev_id Device ID pointer passed in to request_irq
 * @param  regs   Snapshot of the processor's context before entering interrupt code
 */
static irq_handler_t pfi_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs) {
    struct net_device* device;

    printk(KERN_INFO DRV_NAME ": interrupt received\n");
    device = dev_get_by_name(&init_net, "eth0");
    device->phydev->drv->suspend(device->phydev);

    gpio_set_value(gpio_pin, 1);

    return (irq_handler_t) IRQ_HANDLED;
}

 
/** @brief The LKM initialization function
 *  @return returns 0 if successful
 */
static int __init pfi_init(void){

    int result = gpio_request_one(irq_pin, GPIOF_IN, DRV_NAME " irq");

    /* TEMP for timing tests */
    int err = gpio_request_one(gpio_pin, GPIOF_OUT_INIT_HIGH, DRV_NAME " gpio");
    if (err < 0) {
        printk(KERN_ALERT DRV_NAME ": failed to request GPIO pin %d.\n",
               gpio_pin);
    }
    /* TEMP end */

    if (result >= 0)
    {
        irq_number = gpio_to_irq(irq_pin);
        if (irq_number >= 0)
        {
            result = request_threaded_irq(irq_number, NULL, (irq_handler_t) pfi_irq_handler,
                                             IRQF_ONESHOT | IRQF_TRIGGER_FALLING, DRV_NAME "_irq_handler", NULL);
        }
        else
        {
            printk(KERN_ALERT DRV_NAME ": failed to get IRQ for pin %d.\n", irq_pin);
            result = irq_number;
            gpio_free(irq_pin);
        }
    }
    else
    {
        printk(KERN_ALERT DRV_NAME ": failed to request IRQ pin %d.\n", irq_pin);
    }


    printk(KERN_INFO DRV_NAME ": initialization %s\n", result < 0 ? "failed" : "succeeded");
    return result;
}
 
/** @brief The LKM cleanup function
 */
static void __exit pfi_exit(void){
   free_irq(irq_number, NULL);
    
   gpio_free(irq_pin);
    
   printk(KERN_INFO DRV_NAME ": unloaded\n");
}
 
module_init(pfi_init);
module_exit(pfi_exit);
