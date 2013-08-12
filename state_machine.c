#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/hrtimer.h>

#include "dht22.h"
#include "state_machine.h"

static void startup_pulse(unsigned gpio);
static irqreturn_t handler(int, void*);

static DECLARE_WAIT_QUEUE_HEAD(acquisition);

static void startup_pulse(unsigned gpio) {
  gpio_direction_output(gpio,0);
  usleep_range(500, 2000);
  gpio_direction_input(gpio);
}

int read_dht22(struct device* dev) {
  struct dht22_priv* priv = dev_get_drvdata(dev);
  int r;
  
  int irq = gpio_to_irq(priv->gpio);
  printk(KERN_DEBUG "irq: %u", irq);
  
  startup_pulse(priv->gpio);
  
  priv->state = START;
  request_irq(irq, handler, IRQF_TRIGGER_FALLING, dev_name(dev), priv);

  trace_printk("Before wait\n");
  if(wait_event_interruptible_timeout(acquisition, priv->state==DONE, msecs_to_jiffies(100)) > 0)
    r = 0;
  else
    r = -1;
  trace_printk("After wait\n");
  
  free_irq(irq, priv);
  return r;
}

static irqreturn_t handler(int irq, void* dev_id) {
  ktime_t currTime = ktime_get();
  trace_printk("Fire in the hole !/n");
  struct dht22_priv* priv = (struct dht22_priv*)dev_id;
  s64 timeDiff;
  
  switch(priv->state) {
  case START:
    priv->state = WARMUP;
    break;
  case WARMUP:
    priv->state = DATA_READ;
    break;
  case DATA_READ:
    timeDiff = ktime_us_delta(currTime,priv->lastIntTime);
    
    int currBit = (timeDiff < 98) ? 0 : 1;
    priv->data[priv->byteCount] &= currBit << (7 - priv->bitCount);
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
    printk(KERN_ERR "Interrput occured while state is DONE\n");
  }
  
  priv->lastIntTime = currTime;
  return IRQ_HANDLED;
}
