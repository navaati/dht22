#ifndef H_STATE_MACHINE
#define H_STATE_MACHINE

#include <linux/interrupt.h>
#include "dht22.h"

int read_dht22(struct device* dev);
irqreturn_t dht22_handler(int irq, void* dev_id);

#endif /* H_STATE_MACHINE */
