#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vorne Industries");
MODULE_DESCRIPTION("Take emergency actions on power fail interrupt");
MODULE_VERSION("0.1");
 
/** @brief The LKM initialization function
 *  @return returns 0 if successful
 */
static int __init pfi_init(void){
   printk(KERN_INFO "pfi: initialized\n");
   return 0;
}
 
/** @brief The LKM cleanup function
 */
static void __exit pfi_exit(void){
}
 
module_init(pfi_init);
module_exit(pfi_exit);
