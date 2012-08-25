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
#include "p_ecm.h"
#include "p_ecm_msc.h"
#include "p_msc.h"

static int personality_ecm_msc_bind_config(struct usb_configuration *c)
{
  int result = personality_ecm_bind_config(c);
  if (!result) {
    result = personality_msc_bind_config(c);
    if (!result) {
      return 0;
    }
    personality_ecm_unbind_config(c);
  }
  return result;
}

static void personality_ecm_msc_unbind_config(struct usb_configuration *c)
{
  personality_msc_unbind_config(c);
  personality_ecm_unbind_config(c);
}

static int personality_adb_ecm_msc_bind_config(struct usb_configuration *c)
{
  int result = personality_ecm_msc_bind_config(c);
  if (!result) {
    result = personality_adb_bind_config(c);
    if (!result) {
      return 0;
    }
    personality_ecm_msc_unbind_config(c);
  }
  return result;
}

void add_ecm_msc_configurations(struct usb_composite_dev* cdev, const char* name)
{
  static struct usb_configuration config = {
    .bind		= personality_ecm_msc_bind_config,
    .unbind		= personality_ecm_msc_unbind_config,
    .bConfigurationValue = 1,
    .bmAttributes	= USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
  };
  config.label = name;
  usb_add_config(cdev, &config);
}

void add_adb_ecm_msc_configurations(struct usb_composite_dev* cdev, const char* name)
{
  static struct usb_configuration config = {
    .bind		= personality_adb_ecm_msc_bind_config,
    .unbind		= personality_ecm_msc_unbind_config, /* No adb-specific cleanup. */
    .bConfigurationValue = 1,
    .bmAttributes	= USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
  };
  config.label = name;
  usb_add_config(cdev, &config);
}
