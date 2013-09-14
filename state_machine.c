#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/hrtimer.h>
#include <linux/string.h>

#include "dht22.h"
#include "state_machine.h"

static void startup_pulse(unsigned gpio);

static DECLARE_WAIT_QUEUE_HEAD(acquisition);

static void startup_pulse(unsigned gpio) {
  gpio_direction_output(gpio,0);
  usleep_range(500, 2000);
  gpio_direction_input(gpio);
}

int read_dht22(struct device* dev) {
  struct dht22_priv* priv;
  int r;
  
  priv = dev_get_drvdata(dev);
  
  priv->lastIntTime = ktime_get();
  priv->state = READY;
  memset(priv->data,0,5);
  priv->bitCount = 7;
  priv->byteCount = 0;
  
  startup_pulse(priv->gpio);
 
  if(wait_event_interruptible_timeout(acquisition, priv->state==DONE, msecs_to_jiffies(100)) > 0)
    r = 0;
  else
    r = -1;
  
  return r;
}

irqreturn_t dht22_handler(int irq, void* dev_id) {
  ktime_t currTime;
  struct device* dev;
  struct dht22_priv* priv;
  s64 timeDiff;
  int currBit;
  
  currTime = ktime_get();
  dev = (struct device*)dev_id;
  priv = dev_get_drvdata(dev);
  timeDiff = ktime_us_delta(currTime,priv->lastIntTime);
  
  trace_printk("tD = %lld us, state = %d, byte.bit = %u.%u, data = %x:%x:%x:%x:%x\n", (long long)timeDiff, priv->state, priv->byteCount, priv->bitCount, priv->rh_int, priv->rh_dec, priv->t_int, priv->t_dec, priv->checksum);
  
  switch(priv->state) {
  case READY:
    priv->state = START;
    break;
  case START:
    priv->state = WARMUP;
    break;
  case WARMUP:
    priv->state = DATA_READ;
    break;
  case DATA_READ:
    currBit = (timeDiff < 100) ? 0 : 1;
    priv->data[priv->byteCount] |= currBit << priv->bitCount;
    priv->bitCount--;
    
    if (priv->bitCount < 0) {
      priv->byteCount++;
      priv->bitCount = 7;
    }
    
    if (priv->byteCount > 4) {
      priv->state = DONE;
      wake_up(&acquisition);
    }
    
    if (timeDiff > 140)
      currTime = ktime_sub_us(currTime, timeDiff-140);
    
    break;
  case DONE:
    dev_err(dev, "Interrupt occured while state is DONE\n");
  }
  
  priv->lastIntTime = currTime;
  return IRQ_HANDLED;
}
