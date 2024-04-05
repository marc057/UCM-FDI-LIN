#include <asm-generic/errno.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>


#define ALL_LEDS_ON 0x7
#define ALL_LEDS_OFF 0

#define MANUAL_DEBOUNCE

#define NR_GPIO_LEDS 3
#define NR_GPIO_BUTTONS 3

#define DEVICE_NAME "ledsw"

MODULE_DESCRIPTION("Ledsw");
MODULE_AUTHOR("Marcos Colombas");
MODULE_LICENSE("GPL");

DEFINE_MUTEX(mtx); // Mutex to protect curr_led_state

const int led_gpio[NR_GPIO_LEDS] = {25, 27, 4};
const int button_gpio[NR_GPIO_BUTTONS] = {9, 10, 22};

typedef struct
{
  unsigned char curr_led_state;
  int switch_irq_n[NR_GPIO_BUTTONS];
  struct gpio_desc *desc_leds[NR_GPIO_LEDS];
  struct gpio_desc *desc_button[NR_GPIO_BUTTONS];
} gpio_state;

static gpio_state gstate;

/* Set led state to that specified by mask */
static inline int set_pi_leds(unsigned int mask)
{
  int i;
  for (i = 0; i < NR_GPIO_LEDS; i++)
    gpiod_set_value(gstate.desc_leds[i], (mask >> i) & 0x1);
  return 0;
}

/* Interrupt handler for button **/
static irqreturn_t gpio_irq_handler(int irq, void *dev_id)
{
  int i;

#ifdef MANUAL_DEBOUNCE
  static unsigned long last_interrupt = 0;
  unsigned long diff = jiffies - last_interrupt;
  if (diff < 20)
    return IRQ_HANDLED;

  last_interrupt = jiffies;
#endif

  
  for (i = 0; i < NR_GPIO_BUTTONS; i++)
  {
    if (irq == gstate.switch_irq_n[i])
    {
      mutex_lock(&mtx);
      gstate.curr_led_state ^= (1 << i); // Toggle the corresponding bit
      mutex_unlock(&mtx);
      set_pi_leds(gstate.curr_led_state);
      break;
    }
  }

  return IRQ_HANDLED;
}

/* Intercambia los bits 2 y 0*/
static inline unsigned int swap_mask(unsigned int mask)
{
  unsigned int bit0 = mask & 1;
  unsigned int bit2 = (mask >> 2) & 1;

  mask ^= (-bit0 ^ mask) & (1 << 2);
  mask ^= (-bit2 ^ mask) & (1 << 0);
  return mask;
}

static ssize_t ledsw_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
  char kbuf[2];
  int len;

  mutex_lock(&mtx);
  kbuf[0] = gstate.curr_led_state + '0';
  kbuf[1] = '\n';
  mutex_unlock(&mtx);

  len = min(count, sizeof(kbuf));

  if (copy_to_user(buf, kbuf, len))
    return -EFAULT;

  return len;
}

static ssize_t ledsw_write(struct file *file, const char __user *buf, size_t len, loff_t *off)
{
  char *kbuf;
  unsigned int mask;
  int errorcode;

  kbuf = kmalloc(len + 1, GFP_KERNEL);
  if (!kbuf)
    return -ENOMEM;

  if (copy_from_user(kbuf, buf, len))
  {
    kfree(kbuf);
    return -EFAULT;
  }
  kbuf[len] = '\0';

  errorcode = kstrtouint(kbuf, 10, &mask);
  if (errorcode)
  {
    kfree(kbuf);
    return errorcode;
  }

  if (mask < 0 || mask > 7)
  {
    kfree(kbuf);
    return -EINVAL;
  }

  mask = swap_mask(mask);
  mutex_lock(&mtx);
  gstate.curr_led_state = (char)mask; // Update led state
  mutex_unlock(&mtx);
  set_pi_leds(mask);

  kfree(kbuf);

  return len;
}

static const struct file_operations ledsw_fops = {
    .owner = THIS_MODULE,
    .write = ledsw_write,
    .read = ledsw_read,
};

static struct miscdevice ledsw_miscdev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEVICE_NAME,
    .mode = 0666,
    .fops = &ledsw_fops,
};

static int __init gpioint_init(void)
{
  int i, j;
  int err = 0;
  char gpio_str[10];
  unsigned char gpio_out_ok = 0;

  mutex_init(&mtx);

  if (misc_register(&ledsw_miscdev))
  {
    pr_err("Couldn't register misc device\n");
    goto err_handle;
  }

  for (i = 0; i < NR_GPIO_LEDS; i++)
  {
    /* Build string ID */
    sprintf(gpio_str, "led_%d", i);
    // Requesting the GPIO
    if ((err = gpio_request(led_gpio[i], gpio_str)))
    {
      pr_err("Failed GPIO[%d] %d request\n", i, led_gpio[i]);
      goto err_handle;
    }

    /* Transforming into descriptor **/
    if (!(gstate.desc_leds[i] = gpio_to_desc(led_gpio[i])))
    {
      pr_err("GPIO[%d] %d is not valid\n", i, led_gpio[i]);
      err = -EINVAL;
      goto err_handle;
    }

    gpiod_direction_output(gstate.desc_leds[i], 0);
  }

  for (i = 0; i < NR_GPIO_BUTTONS; i++)
  {
    /* Requesting Button's GPIO */
    if ((err = gpio_request(button_gpio[i], "button")))
    {
      pr_err("ERROR: GPIO %d request\n", button_gpio[i]);
      goto err_handle;
    }

    /* Configure Button */
    if (!(gstate.desc_button[i] = gpio_to_desc(button_gpio[i])))
    {
      pr_err("GPIO %d is not valid\n", button_gpio[i]);
      err = -EINVAL;
      goto err_handle;
    }

    gpio_out_ok = 1;

    // configure the BUTTON GPIO as input
    gpiod_direction_input(gstate.desc_button[i]);

    /*
    ** The lines below are commented because gpiod_set_debounce is not supported
    ** in the Raspberry pi. Debounce is handled manually in this driver.
    */
#ifndef MANUAL_DEBOUNCE
    // Debounce the button with a delay of 200ms
    if (gpiod_set_debounce(gstate.desc_button[i], 200) < 0)
    {
      pr_err("ERROR: gpio_set_debounce - %d\n", button_gpio[i]);
      goto err_handle;
    }

#endif

    // Get the IRQ number for our GPIO
    gstate.switch_irq_n[i] = gpiod_to_irq(gstate.desc_button[i]);
    pr_info("IRQ Number for Button %d = %d\n", i, gstate.switch_irq_n[i]);

    if (request_irq(gstate.switch_irq_n[i], // IRQ number
                    gpio_irq_handler,       // IRQ handler
                    IRQF_TRIGGER_RISING,    // Handler will be called in raising edge
                    "button_leds",          // used to identify the device name using this IRQ
                    NULL))
    { // device id for shared IRQ
      pr_err("my_device: cannot register IRQ ");
      goto err_handle;
    }
  }

  gstate.curr_led_state = ALL_LEDS_ON;
  set_pi_leds(ALL_LEDS_ON);
  return 0;

err_handle:
  for (j = 0; j < NR_GPIO_LEDS; j++)
    gpiod_put(gstate.desc_leds[j]);

  for (j = 0; j < i; j++)
    gpiod_put(gstate.desc_button[j]);

  misc_deregister(&ledsw_miscdev);

  mutex_destroy(&mtx);

  return err;
}

static void __exit gpioint_exit(void)
{
  int i;

  for (i = 0; i < NR_GPIO_BUTTONS; i++)
    free_irq(gstate.switch_irq_n[i], NULL);

  set_pi_leds(ALL_LEDS_OFF);

  misc_deregister(&ledsw_miscdev);

  for (i = 0; i < NR_GPIO_LEDS; i++)
    gpiod_put(gstate.desc_leds[i]);

  for (i = 0; i < NR_GPIO_BUTTONS; i++)
    gpiod_put(gstate.desc_button[i]);

  mutex_destroy(&mtx);
}

module_init(gpioint_init);
module_exit(gpioint_exit);
