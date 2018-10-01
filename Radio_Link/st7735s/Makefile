CONFIG_MODULE_SIG=n

obj-m := st7735s.o

KDIR ?= ${BUILD_KERNEL}

PWD := $(shell pwd)

default:
		make ARCH=arm -C $(HOME)/build/ M=$(PWD) modules
 		#$(MAKE) ARCH=arm -C $(KDIR) M=$(PWD) modules
clean:
		  $(MAKE) ARCH=arm  -C $(KDIR) SUBDIRS=$(PWD) clean

config:
		$(MAKE) ARCH=arm -C $(KDIR) menuconfig

dtb:
		make ARCH=arm -C $(HOME)/linux-4.18.10/ dtbs

.PHONY: clean default config dtb
