KDIR=/lib/modules/$(shell uname -r)/build

obj-m += calc.o
obj-m += livepatch-calc.o
calc-objs += main.o expression.o
ccflags-y := -std=gnu99 -Wno-declaration-after-statement

GIT_HOOKS := .git/hooks/applied

all: $(GIT_HOOKS)
	make -C $(KDIR) M=$(PWD) modules

$(GIT_HOOKS):
	@scripts/install-git-hooks
	@echo

load:
	sudo insmod calc.ko
	sudo chmod -R 0666 /dev/calc

unload: unpatch
	-sudo rmmod calc

reload: unload load

patch:
	sudo insmod livepatch-calc.ko

unpatch:
	-sudo sh -c "echo 0 > /sys/kernel/livepatch/livepatch_calc/enabled"
	sleep 2
	-sudo rmmod livepatch-calc

repatch: unpatch patch

status:
	lsmod | grep calc

check: all
	scripts/test.sh

clean:
	make -C $(KDIR) M=$(PWD) clean
