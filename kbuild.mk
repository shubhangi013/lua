ifneq ($(KERNELRELEASE),)
    obj-m := luanotifier.o
else
    KERNELDIR ?= /lib/modules/$(shell uname -r)/build
    PWD := $(shell pwd)
    MAKE := /usr/bin/make

default:
	@echo "Building kernel module..."
	@echo "Kernel directory: $(KERNELDIR)"
	@echo "Current directory: $(PWD)"
	@echo "Source files: $(wildcard *.c)"
	@echo "Using make: $(MAKE)"
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules V=1 SHELL=/bin/bash

clean:
	@echo "Cleaning build files..."
	rm -f *.ko *.mod.c *.mod.o *.o *.order *.symvers
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
endif 