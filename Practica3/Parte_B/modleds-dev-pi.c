#include <linux/module.h>                                                       
#include <asm-generic/errno.h>                                                  
#include <linux/gpio.h>                                                         
#include <linux/fs.h>                                                           
#include <linux/cdev.h>                                                         
#include <linux/delay.h>                                                        
#include <linux/uaccess.h>                                                      
#include <linux/string.h>                                                       
#include <linux/slab.h>
#include <linux/miscdevice.h>

#define ALL_LEDS_ON 0x7
#define ALL_LEDS_OFF 0
#define NR_GPIO_LEDS 3
#define DEVICE_NAME "leds"

MODULE_DESCRIPTION("ModledsPi_gpiod Kernel Module - FDI-UCM");
MODULE_AUTHOR("Marcos Colombas, German Atalyanzs");
MODULE_LICENSE("GPL");

static struct miscdevice modleds_miscdev;

/* Actual GPIOs used for controlling LEDs */                                    
const int led_gpio[NR_GPIO_LEDS] = {25, 27, 4};                                 
                                                                                
/* Array to hold gpio descriptors */                                            
struct gpio_desc *gpio_descriptors[NR_GPIO_LEDS];                               
                                                                                
/* Set led state to that specified by mask */                                   
static inline int set_pi_leds(unsigned int mask) {                              
  int i;                                                                        
  for (i = 0; i < NR_GPIO_LEDS; i++)                                            
    gpiod_set_value(gpio_descriptors[i], (mask >> i) & 0x1);                    
  return 0;                                                                     
}                                                                               
                                                                                
/* Intercambia los bits 2 y 0*/                                                 
static inline unsigned int swap_mask(unsigned int mask) {                       
        unsigned int bit0 = mask & 1;                                           
        unsigned int bit2 = (mask >> 2) & 1;                                    
                                                                                
        mask ^= (-bit0 ^ mask) & (1 << 2);                                      
        mask ^= (-bit2 ^ mask) & (1 << 0);                                      
        return mask;                                                            
}                                                                               
                                                                                
static ssize_t modleds_write(struct file *file, const char __user *buf, size_t l
en, loff_t *off) {                                                              
  char *kbuf;                                                                   
  unsigned int mask;                                                            
  int errorcode;                                                                
                                                                                
  kbuf = kmalloc(len + 1, GFP_KERNEL);                                          
  if (!kbuf)                                                                    
    return -ENOMEM;                                                             
                                                                                
  if (copy_from_user(kbuf, buf, len)) {                                         
    kfree(kbuf);                                                                
    return -EFAULT;                                                             
  }                                                                             
  kbuf[len] = '\0';                                                             
                                                                                
  errorcode = kstrtouint(kbuf, 10, &mask);                                      
  if (errorcode) {                                                              
    kfree(kbuf);                                                                
    return errorcode;                                                           
  }                                                                             
                                                                                
  if (mask < 0 || mask > 7) {                                                   
    kfree(kbuf);                                                                
    return -EINVAL;                                                             
  }                                                                             
                                                                                
  mask = swap_mask(mask);                                                       
  set_pi_leds(mask);                                                            
  kfree(kbuf);                                                                  
                                                                                
  return len;                                                                   
}                                                                               
                                                                                
static const struct file_operations modleds_fops = {                            
    .owner = THIS_MODULE,                                                       
    .write = modleds_write,                                                     
};                                                                              
                                                                                
static struct miscdevice modleds_miscdev = {                                    
        .minor = MISC_DYNAMIC_MINOR,                                            
        .name = DEVICE_NAME,                                                    
        .mode = 0666,                                                           
        .fops = &modleds_fops,                                                  
};                                                                              
                                                                                
static int __init modleds_init(void) {                                          
  int i, j;                                                                     
  int err = 0;                                                                  
  char gpio_str[10];                                                            
                                                                                
  if (misc_register(&modleds_miscdev)) {                                        
    pr_err("Couldn't register misc device\n");                                  
    goto err_handle;                                                            
  }                                                                             
                                                                                
  for (i = 0; i < NR_GPIO_LEDS; i++) {                                          
    /* Build string ID */                                                       
    sprintf(gpio_str, "led_%d", i);                                             
    /* Request GPIO */                                                          
    if ((err = gpio_request(led_gpio[i], gpio_str))) {                          
      pr_err("Failed GPIO[%d] %d request\n", i, led_gpio[i]);                   
      goto err_handle;                                                          
    }                                                                           
                                                                                
    /* Transforming into descriptor */                                          
    if (!(gpio_descriptors[i] = gpio_to_desc(led_gpio[i]))) {                   
      pr_err("GPIO[%d] %d is not valid\n", i, led_gpio[i]);                     
      err = -EINVAL;                                                            
      goto err_handle;                                                          
    }                                                                           
                                                                                
    gpiod_direction_output(gpio_descriptors[i], 0);                             
  }                                                                             
                                                                                
  set_pi_leds(ALL_LEDS_ON);                                                     
  msleep(1000);                                                                    
  set_pi_leds(ALL_LEDS_OFF);                                                    
                                                                                
  return 0;                                                                     
                                                                                
err_handle:                                                                     
  for (j = 0; j < i; j++)                                                       
    gpiod_put(gpio_descriptors[j]);                                             
                                                                                
  misc_deregister(&modleds_miscdev);                                            
  return err;                                                                   
}                                                                               
                                                                                
static void __exit modleds_exit(void) {                                         
  int i = 0;                                                                    
                                                                                
  set_pi_leds(ALL_LEDS_OFF);                                                    
                                                                                
  misc_deregister(&modleds_miscdev);                                            
                                                                                
  for (i = 0; i < NR_GPIO_LEDS; i++)                                            
    gpiod_put(gpio_descriptors[i]);                                             
}                                                                               
                                                                                
module_init(modleds_init);                                                      
module_exit(modleds_exit);                                                      
                                                                                
MODULE_LICENSE("GPL");                                                          
MODULE_DESCRIPTION("Modleds");  