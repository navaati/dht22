#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>

#include "dht22.h"
#include "state_machine.h"

static ssize_t dht22_show_temperature(struct device *dev, struct device_attribute *attr, char *buf) {
  struct dht22_priv* priv = dev_get_drvdata(dev);
  
  if (read_dht22(dev)) {
    printk(KERN_ERR "BIG FAIL NRNRNR ! State: %u, bitCount: %u\n", priv->state, priv->bitCount);
    return -EIO;
  }
  
  int temp = 0; /* TODO: Calculate stuff here */
  
  return sprintf(buf, "%d\n", temp);
}

static SENSOR_DEVICE_ATTR(temp1_input, S_IRUGO, dht22_show_temperature, NULL, 0);
/*static SENSOR_DEVICE_ATTR(humidity1_input, S_IRUGO, dht22_show_humidity, NULL, 0);*/

static struct attribute* dht22_attributes[] = {
  &sensor_dev_attr_temp1_input.dev_attr.attr,
  /*&sensor_dev_attr_humidity1_input.dev_attr.attr,*/
  NULL
};

static const struct attribute_group dht22_attr_group = {
  .attrs = dht22_attributes,
};

static int dht22_probe(struct platform_device *pdev) { 
  struct dht22_platform_data* pdata = pdev->dev.platform_data;
  struct dht22_priv* priv;
  
  priv = devm_kzalloc(&pdev->dev,sizeof(struct dht22_priv),GFP_KERNEL);
  if (!priv)
    return -ENOMEM;

  if (pdata)
    priv->gpio = pdata->gpio;
  else
    priv->gpio = of_get_gpio(pdev->dev.of_node, 0);
  printk(KERN_INFO "Creating dht22 for gpio %d\n", priv->gpio);
  
  if(!gpio_is_valid(priv->gpio)) {
    printk(KERN_ERR "Invalid GPIO number : %u",priv->gpio);
    return -EINVAL;
  }
  if(gpio_cansleep(priv->gpio)) {
    printk(KERN_ERR "Calls for GPIO number %u can sleep, driver can't use it",priv->gpio);
    return -EFAULT;
  }
  if (gpio_request_one(priv->gpio, GPIOF_IN, dev_name(&pdev->dev))) {
    printk(KERN_ERR "Can't request GPIO number %u",priv->gpio);
    return -EFAULT;
  }
  
  sysfs_create_group(&pdev->dev.kobj, &dht22_attr_group);
  
  platform_set_drvdata(pdev,priv);
  
  printk("Created device %s\n", dev_name(&pdev->dev));
  return 0;
}

static int dht22_remove(struct platform_device *pdev) {
  struct dht22_priv *priv = platform_get_drvdata(pdev);
  
  gpio_direction_input(priv->gpio);
  gpio_free(priv->gpio);
  
  sysfs_remove_group(&pdev->dev.kobj, &dht22_attr_group);
  
  platform_set_drvdata(pdev, NULL);
  
  return 0;
}

static const struct of_device_id dht22_of_ids[] = {
  { .compatible = "aosong,dht22" },
  {}
};
MODULE_DEVICE_TABLE(of, dht22_of_ids);

static struct platform_driver dht22_driver = {
  .probe = dht22_probe,
  .remove = dht22_remove,
  .driver = {
    .name = "dht22",
    .owner = THIS_MODULE,
    .of_match_table = dht22_of_ids
  }
};

module_platform_driver(dht22_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Léo Gillot-Lamure");
