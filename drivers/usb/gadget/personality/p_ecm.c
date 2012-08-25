/* Copyright (C) 2011 emsys Embedded Systems GmbH
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include "disable_init_defines.h"
#include "../f_ecm.c"

int personality_ecm_bind_config(struct usb_configuration *c)
{
  u8 ethaddr[ETH_ALEN];
  int result = gether_setup(c->cdev->gadget, /* out */ ethaddr);
  if (!result) {
    result = ecm_bind_config(c, /* by value */ ethaddr);
    if (!result) {
      return 0;
    } else {
      printk(KERN_WARNING "%s: ecm_bind_config failed (%d)\n", __FUNCTION__, result);
    }
    gether_cleanup();
  } else {
    printk(KERN_WARNING "%s: gether_setup failed (%d)\n", __FUNCTION__, result);
  }
  return result;
}

void personality_ecm_unbind_config(struct usb_configuration *c)
{
  printk(KERN_DEBUG "%s()\n", __FUNCTION__);
  gether_cleanup();
}

void add_ecm_configurations(struct usb_composite_dev* cdev, const char* name)
{
  static struct usb_configuration config = {
    .bind		= personality_ecm_bind_config,
    .unbind		= personality_ecm_unbind_config,
    .bConfigurationValue = 1,
    .bmAttributes	= USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
  };
  config.label = name;
  usb_add_config(cdev, &config);
}
