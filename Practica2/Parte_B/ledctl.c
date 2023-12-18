#include <linux/module.h>
#include <asm-generic/errno.h>
#include <linux/init.h>
#include <linux/tty.h>      /* For fg_console */
#include <linux/kd.h>       /* For KDSETLED */
#include <linux/vt_kern.h>
#include <linux/syscalls.h> /* For the new syscall */
#include <linux/kernel.h>	/* For the new syscall */

#define ALL_LEDS_ON 0x7
#define ALL_LEDS_OFF 0

/* Get driver handler */
struct tty_driver* get_kbd_driver_handler(void){
   printk(KERN_INFO "modleds: loading\n");
   printk(KERN_INFO "modleds: fgconsole is %x\n", fg_console);
   return vc_cons[fg_console].d->port.tty->driver;
}

/* Set led state to that specified by mask */
static inline int set_leds(struct tty_driver* handler, unsigned int mask){
    return (handler->ops->ioctl) (vc_cons[fg_console].d->port.tty, KDSETLED,mask);
}

unsigned int mod_mask(unsigned int leds) {
    int bit2 = leds & (0x1 << 2);
    int bit1 = leds & (0x1 << 1);

    return ((leds & 0x1) | (bit1 << 1) | (bit2 >> 1));
}

SYSCALL_DEFINE1(ledctl, unsigned int, leds) {
    unsigned int mask;
    struct tty_driver* kbd_driver= get_kbd_driver_handler();

	if ((leds < ALL_LEDS_OFF) || (leds > ALL_LEDS_ON))
		return -EINVAL;

    mask = mod_mask(leds);

	return set_leds(kbd_driver, mask);
}
