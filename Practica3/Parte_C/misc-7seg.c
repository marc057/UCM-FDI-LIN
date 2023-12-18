#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <asm-generic/errno.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
                                                                                
MODULE_DESCRIPTION("Misc Display7s Kernel Module - FDI-UCM");                   
MODULE_AUTHOR("Juan Carlos Saez");                                              
MODULE_LICENSE("GPL");                                                          
                                                                                
/* Max buffer size */                                                           
                                                                                
#define BUFFER_LENGTH   10                                                    
                                                                                
/* Bits associated with each segment */                                         
#define DS_A 0x80                                                               
#define DS_B 0x40                                                               
#define DS_C 0x20                                                               
#define DS_D 0x10                                                               
#define DS_E 0x08                                                               
#define DS_F 0x04                                                               
#define DS_G 0x02                                                               
#define DS_DP 0x01                                                              
#define SEGMENT_COUNT 8                                                         
                                                                                
/* Indices of GPIOs used by this module */                                      
enum                                                                            
{                                                                               
        SDI_IDX = 0,                                                            
        RCLK_IDX,                                                               
        SRCLK_IDX,                                                              
        NR_GPIO_DISPLAY                                                         
};                                                                              
                                                                                
/* Pin numbers */                                                               
const int display_gpio[NR_GPIO_DISPLAY] = {18, 23, 24};                         
                                                                                
/* Array to hold GPIO descriptors */                                            
struct gpio_desc *gpio_descriptors[NR_GPIO_DISPLAY];                            
                                                                                
const char *display_gpio_str[NR_GPIO_DISPLAY] = {"sdi", "rclk", "srclk"};       
                                                                                
/* Sequence of segments used by the character device driver */                  
const int sequence[] = {DS_D, DS_E, DS_F, DS_A, DS_B, DS_C, DS_G, DS_DP, -1};   
                                                                                
#define DEVICE_NAME "display7s" /* Device name */                               
                                                                                
/*                                                                              
 *  Prototypes                                                                  
 */                                                                             
static ssize_t display7s_write(struct file *, const char *, size_t, loff_t *);  
                                                                                
/* Simple initialization of file_operations interface with a single operation */
static struct file_operations fops = {                                          
        .write = display7s_write,                                               
};                                                                              
                                                                                
static struct miscdevice display7s_misc = {                                     
        .minor = MISC_DYNAMIC_MINOR, /* kernel dynamically assigns a free minor#
 */                                                                             
        .name = DEVICE_NAME,             /* when misc_register() is invoked, the
 kernel                                                                         
                                                                  * will auto-cr
eate device file;                                                               
                                                                  * also populat
ed within /sys/class/misc/ and /sys/devices/virtual/misc/ */                    
        .mode = 0666,                            /* ... dev node perms set as sp
ecified here */                                                                 
        .fops = &fops,                           /* connect to this driver's 'fu
nctionality' */                                                                 
};                                                                              
                                                                                
                                                                                
/* Update the 7-segment display with the configuration specified by the data par
ameter */                                                                       
static void update_7sdisplay(unsigned char data)                                
{                                                                               
        int i = 0;                                                              
        int value = 0;                                                          
                                                                                
        for (i = 0; i < SEGMENT_COUNT; i++)                                     
        {                                                                       
                /* Explore current bit (from most significant to least significa
nt) */                                                                          
                if (0x80 & (data << i))                                         
                        value = 1;                                              
                else                                                            
                        value = 0;                                              
                                                                                
                /* Set value of serial input */                                 
                gpiod_set_value(gpio_descriptors[SDI_IDX], value);              
                /* Generate clock cycle in shift register */                    
                gpiod_set_value(gpio_descriptors[SRCLK_IDX], 1);                
                msleep(1);                                                      
                gpiod_set_value(gpio_descriptors[SRCLK_IDX], 0);                
        }                                                                       
                                                                                
        /* Generate clock cycle in output register to update 7-seg display */   
        gpiod_set_value(gpio_descriptors[RCLK_IDX], 1);                         
        msleep(1);                                                              
        gpiod_set_value(gpio_descriptors[RCLK_IDX], 0);                         
}                                                                               
                                                                                
unsigned int char_to_max(char c) {                                              
        switch (c) {                                                            
                case '0':                                                       
                        return 252;                                             
                case '1':                                                       
                        return 96;                                              
                case '2':                                                       
                        return 218;                                             
                case '3':                                                       
                        return 242;                                             
                case '4':                                                       
                        return 102;                                             
                case '5':                                                       
                        return 182;                                             
                case '6':                                                       
                        return 190;                                             
                case '7':                                                       
                        return 224;                                             
                case '8':                                                       
                        return 254;                                             
                case '9':                                                       
                        return 246;                                             
                case 'a':                                                       
                case 'A':                                                       
                        return 238;                                             
                case 'b':                                                       
                case 'B':                                                       
                        return 62;                                              
                case 'c':                                                       
                case 'C':                                                       
                        return 156;                                             
                case 'd':                                                       
                case 'D':                                                       
                        return 122;                                             
                case 'e':                                                       
                case 'E':                                                       
                        return 158;                                             
                case 'f':                                                       
                case 'F':                                                       
                        return 142;                                             
                default:                                                        
                        return 0;                                               
        }                                                                       
}                                                                               
                                                                                
/*                                                                              
 * Called when a process writes to dev file: echo "hi" > /dev/display7s         
 * Test with the following command to see the sequence better:                  
 * $ while true; do echo > /dev/display7s; sleep 0.3; done                      
 */                                                                             
static ssize_t                                                                  
display7s_write(struct file *filp, const char *buff, size_t len, loff_t *off)   
{                                                                               
        char *kbuf;                                                             
        int mask;                                                               
                                                                                
            /* Reservamos memoria tambien para kbuf*/                           
    kbuf = kmalloc(BUFFER_LENGTH, GFP_DMA);                                     
    if (!kbuf) {                                                                
                kfree(kbuf);                                                    
                return -ENOMEM;                                                 
        }                                                                       
                                                                                
                                                                                
    //  Copiamos cadena alojada en user_buffer a buffer auxiliar (kbuf)         
    if (copy_from_user(kbuf, buff, len)) {                                      
                kfree(kbuf);                                                    
        return -EFAULT;                                                         
        }                                                                       
                                                                                
        kbuf[len] = '\0';                                                       
                                                                                
        if (strlen(kbuf) != 2){                                                 
                kfree(kbuf);                                                    
                return -EINVAL;                                                 
        }                                                                       
                                                                                
        mask = char_to_max(kbuf[0]);                                            
        if (mask) {                                                             
                /* Update the corresponding value in the display */             
        update_7sdisplay(mask);                                                 
        return len;                                                             
        }                                                                       
                                                                                
        kfree(kbuf);                                                            
        return -EINVAL;                                                         
}                                                                               
                                                                                
static int __init display7s_misc_init(void)                                     
{                                                                               
        int i, j;                                                               
        int err = 0;                                                            
        struct device *device;                                                  
                                                                                
        for (i = 0; i < NR_GPIO_DISPLAY; i++)                                   
        {                                                                       
                                                                                
                /* Request the GPIO */                                          
                if ((err = gpio_request(display_gpio[i], display_gpio_str[i]))) 
                {                                                               
                        pr_err("Failed GPIO[%d] %d request\n", i, display_gpio[i
]);                                                                             
                        goto err_handle;                                        
                }                                                               
                                                                                
                /* Transform number into descriptor */                          
                if (!(gpio_descriptors[i] = gpio_to_desc(display_gpio[i])))     
                {                                                               
                        pr_err("GPIO[%d] %d is not valid\n", i, display_gpio[i])
;                                                                               
                        err = -EINVAL;                                          
                        goto err_handle;                                        
                }                                                               
                                                                                
                /* Configure as an output pin */                                
                gpiod_direction_output(gpio_descriptors[i], 0);                 
        }                                                                       
                                                                                
        /* Set everything as LOW */                                             
        for (i = 0; i < NR_GPIO_DISPLAY; i++)                                   
                gpiod_set_value(gpio_descriptors[i], 0);                        
                                                                                
        /* Register misc device that exposes 7-segment display */               
        err = misc_register(&display7s_misc);                                   
                                                                                
        if (err)                                                                
        {                                                                       
                pr_err("Couldn't register misc device\n");                      
                goto err_handle;                                                
        }                                                                       
                                                                                
        device = display7s_misc.this_device;                                    
                                                                                
        dev_info(device, "Display7s driver registered succesfully. To talk to\n"
);                                                                              
        dev_info(device, "the driver try to cat and echo to /dev/%s.\n", DEVICE_
NAME);                                                                          
        dev_info(device, "Remove the module when done.\n");                     
                                                                                
        return 0;                                                               
err_handle:                                                                     
        for (j = 0; j < i; j++)                                                 
                gpiod_put(gpio_descriptors[j]);                                 
        return err;                                                             
}                                                                               
                                                                                
static void __exit display7s_misc_exit(void)                                    
{                                                                               
        int i = 0;                                                              
                                                                                
        /* Unregister character device */                                       
        misc_deregister(&display7s_misc);                                       
                                                                                
        /* Clear display */                                                     
        update_7sdisplay(0);                                                    
                                                                                
        /* Free up pins */                                                      
        for (i = 0; i < NR_GPIO_DISPLAY; i++)                                   
                gpiod_put(gpio_descriptors[i]);                                 
                                                                                
        pr_info("Display7s driver deregistered. Bye\n");                        
}                                                                               
                                                                                
module_init(display7s_misc_init);                                               
module_exit(display7s_misc_exit);                                               
                            