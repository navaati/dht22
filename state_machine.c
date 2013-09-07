#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/hrtimer.h>

#include "dht22.h"
#include "state_machine.h"

static void startup_pulse(unsigned gpio);
static irqreturn_t dht22_handler(int, void*);

static DECLARE_WAIT_QUEUE_HEAD(acquisition);

static void startup_pulse(unsigned gpio) {
  gpio_direction_output(gpio,0);
  usleep_range(500, 2000);
  gpio_direction_input(gpio);
}

int read_dht22(struct device* dev) {
  struct dht22_priv* priv;
  int r;
  int irq;
  
  priv = dev_get_drvdata(dev);
  
  priv->state = START;
  priv->data[0] = 0;
  priv->data[1] = 0;
  priv->data[2] = 0;
  priv->data[3] = 0;
  priv->data[4] = 0;
  priv->bitCount = 0;
  priv->byteCount = 0;

  irq = gpio_to_irq(priv->gpio);
  
  startup_pulse(priv->gpio);
 
  request_irq(irq, dht22_handler, IRQF_TRIGGER_FALLING | IRQF_NO_THREAD, dev_name(dev), dev);

  if(wait_event_interruptible_timeout(acquisition, priv->state==DONE, msecs_to_jiffies(100)) > 0)
    r = 0;
  else
    r = -1;
  
  free_irq(irq, priv);
  return r;
}

static irqreturn_t dht22_handler(int irq, void* dev_id) {
  ktime_t currTime;
  struct device* dev;
  struct dht22_priv* priv;
  s64 timeDiff;
  int currBit;
  
  currTime = ktime_get();
  dev = (struct device*)dev_id;
  priv = dev_get_drvdata(dev);
  timeDiff = ktime_us_delta(currTime,priv->lastIntTime);
  
  trace_printk("tD = %lld us, state = %d, byte.bit = %u.%u, data = %*phC\n", (long long)timeDiff, priv->state, priv->byteCount, priv->bitCount, 5, priv->data);
  
  switch(priv->state) {
  case START:
    priv->state = WARMUP;
    break;
  case WARMUP:
    priv->state = DATA_READ;
    break;
  case DATA_READ:
    
    currBit = (timeDiff < 98) ? 0 : 1;
    priv->data[priv->byteCount] |= currBit << (7 - priv->bitCount);
    priv->bitCount++;
    
    if (priv->bitCount > 7) {
      priv->byteCount++;
      priv->bitCount = 0;
    }
    
    if (priv->byteCount > 4)
      priv->state = DONE;
      wake_up(&acquisition);

    break;
  case DONE:
    dev_err(dev, "Interrupt occured while state is DONE\n");
  }
  
  priv->lastIntTime = currTime;
  return IRQ_HANDLED;
}
