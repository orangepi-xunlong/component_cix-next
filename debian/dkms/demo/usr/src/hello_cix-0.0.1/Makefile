obj-m += hello_cix.o
KVERSION:= $(shell uname -r)

all:
	$(MAKE) -C /lib/modules/$(KVERSION)/build M=$(PWD) modules

clean:
	$(MAKE) -C /lib/modules/$(KVERSION)/build M=$(PWD) clean

install:
	$(MAKE) -C /lib/modules/$(KVERSION)/build M=$(PWD) modules_install
	depmod -a

uninstall:
	rmmode hello_cix
	rm -f /lib/modules/$(KVERSION)/extra/hello_cix.ko
	depmod -a

.PHONY: all clean install uninstall
