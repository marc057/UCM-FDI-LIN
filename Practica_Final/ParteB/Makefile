obj-m += timer.o

## Parte común módulos
ANDROIDSRC=/home/kernel/android-x86-r
KERNEL_OUT=$(ANDROIDSRC)/out/target/product/x86_64/obj/kernel

all:
	make -C $(KERNEL_OUT)  O=$(KERNEL_OUT)  M=$(PWD)  modules

clean:
	make -C $(KERNEL_OUT) O=$(KERNEL_OUT) M=$(PWD)  clean




