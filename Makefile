kv = $(shell uname -r)

dtbo = BB-DHT22-00A2.dtbo

obj-m += dht22.o
dht22-y := dht22_drv.o state_machine.o

all: $(dtbo)
	make -C /lib/modules/$(kv)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(kv)/build M=$(PWD) clean
	rm $(dtbo)

%.dtbo: %.dts
	dtc -O dtb -o $@ -@ $<
