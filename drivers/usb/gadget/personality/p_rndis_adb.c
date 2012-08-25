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

#include "linux/usb.h"
#include "linux/usb/composite.h"

#include "p_adb.h"
#include "p_rndis.h"
#include "p_rndis_adb.h"

static int personality_rndis_adb_bind_config(struct usb_configuration *c)
{
  int result = personality_rndis_bind_config(c);
  if (!result) {
    result = personality_adb_bind_config(c);
    if (!result) {
      return 0;
    }
    personality_rndis_unbind_config(c);
  }
  return result;
}

static void personality_rndis_adb_unbind_config(struct usb_configuration *c)
{
  /* personality_adb_unbind_config(c); */
  personality_rndis_unbind_config(c);
}

void personality_rndis_adb(struct usb_composite_dev* cdev, const char* name)
{
  static struct usb_configuration config = {
    .bind		= personality_rndis_adb_bind_config,
    .unbind		= personality_rndis_adb_unbind_config,
    .setup              = personality_rndis_none_setup,
    .bConfigurationValue = 1,
    .bmAttributes	= USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
  };
  config.label = name;
  usb_add_config(cdev, &config);
}
