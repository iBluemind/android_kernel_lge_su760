/* Copyright (C) 2010, 2011 emsys Embedded Systems GmbH
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/usb/ch9.h>

#include <linux/usb/gadget.h>
#include <linux/usb/composite.h>

#include "disable_init_defines.h"

#include "p_adb.h"
#include "p_msc.h"
#include "p_ecm.h"
#include "p_serial.h"
#include "personality.h"

#include "../u_serial.c" /* for gserial_setup */

typedef enum {
  EVERYTHING,
  SKIP_PREREGISTERED_SER1,
  SKIP_SER2
} tty_opt;

static int  personality_acm_ser_ser_opt_bind_config(struct usb_configuration *c, tty_opt opt)
{
  struct usb_device_descriptor* device_descriptor = personality_cdev_to_device_desc(c->cdev);
  if (device_descriptor) {
    int result = personality_acm_bind_config(c, 0);
    if (0 == result) {
      if (SKIP_PREREGISTERED_SER1 != opt) {
        result = personality_ser_bind_config(c, 1);
      }
      if (0 == result) {
        if (SKIP_SER2 != opt) {
          result = personality_ser_bind_config(c, 2);
        }
        if (0 == result) {
          result = gserial_setup(c->cdev->gadget, SKIP_SER2 == opt ? 2 : 3);
          if (0 == result) {
          	if (EVERYTHING == opt) { // 0x618e
	            device_descriptor->bDeviceClass    = 0x02;
	            device_descriptor->bDeviceSubClass = 0x00;
	            device_descriptor->bDeviceProtocol = 0x00;
			} else {
				device_descriptor->bDeviceClass    = 0xef;
				device_descriptor->bDeviceSubClass = 0x02;
				device_descriptor->bDeviceProtocol = 0x01;
			}
            return 0;
          }
          if (SKIP_SER2 != opt) {
            personality_ser_unbind_config(c, 2);
          }
        }
        if (SKIP_PREREGISTERED_SER1 != opt) {
          personality_ser_unbind_config(c, 1);
        }
      }
      personality_acm_unbind_config(c, 0);
    }
    return result;
  } else {
    return -EINVAL;
  }
}
static void personality_acm_ser_ser_opt_unbind_config(struct usb_configuration *c, tty_opt opt)
{
  struct usb_device_descriptor* device_descriptor = personality_cdev_to_device_desc(c->cdev);
  if (device_descriptor) {
    device_descriptor->bDeviceClass = 0;
    device_descriptor->bDeviceSubClass = 0;
    device_descriptor->bDeviceProtocol = 0;
  }
  gserial_cleanup();
  if (SKIP_SER2 != opt) { personality_ser_unbind_config(c, 2); }
  if (SKIP_PREREGISTERED_SER1 != opt) { personality_ser_unbind_config(c, 1); }
  personality_acm_unbind_config(c, 0);
}

static int  personality_ser_ecm_bind_config(struct usb_configuration *c)
{
  int result = personality_ser_bind_config(c, 1);
  if (0 == result) {
    result = personality_ecm_bind_config(c);
    if (0 == result) {
      return 0;
    }
    personality_ser_unbind_config(c, 1);
  }
  return result;
}
static void personality_ser_ecm_unbind_config(struct usb_configuration *c)
{
  personality_ecm_unbind_config(c);
  personality_ser_unbind_config(c, 1);
}

static int  personality_ser_ecm_acm_ser_bind_config(struct usb_configuration *c)
{
  int result = personality_ser_ecm_bind_config(c);
  if (0 == result) {
    result = personality_acm_ser_ser_opt_bind_config(c, SKIP_PREREGISTERED_SER1);
    if (0 == result) {
      return 0;
    }
    personality_ser_ecm_unbind_config(c);
  }
  return result;
}
static void personality_ser_ecm_acm_ser_unbind_config(struct usb_configuration *c)
{
  personality_acm_ser_ser_opt_unbind_config(c, SKIP_PREREGISTERED_SER1);
  personality_ser_ecm_unbind_config(c);
}

static int  personality_ser_ecm_acm_ser_msc_bind_config(struct usb_configuration *c)
{
  int result = personality_ser_ecm_acm_ser_bind_config(c);
  if (0 == result) {
    result = personality_msc_bind_config(c);
    if (0 == result) {
      return 0;
    }
    personality_ser_ecm_acm_ser_unbind_config(c);
  }
  return result;
}
static void personality_ser_ecm_acm_ser_msc_unbind_config(struct usb_configuration *c)
{
  personality_msc_unbind_config(c);
  personality_ser_ecm_acm_ser_unbind_config(c);
}

static int  personality_ser_ecm_acm_ser_msc_adb_bind_config(struct usb_configuration *c)
{
  int result = personality_ser_ecm_acm_ser_msc_bind_config(c);
  if (0 == result) {
    result = personality_adb_bind_config(c);
    if (0 == result) {
      return 0;
    }
    personality_ser_ecm_acm_ser_msc_unbind_config(c);
  }
  return result;
}
static void personality_ser_ecm_acm_ser_msc_adb_unbind_config(struct usb_configuration *c)
{
  personality_adb_unbind_config(c);
  personality_ser_ecm_acm_ser_msc_unbind_config(c);
}

static int  personality_acm_ser_bind_config(struct usb_configuration *c)
{
  return personality_acm_ser_ser_opt_bind_config(c, SKIP_SER2);
}
static void personality_acm_ser_unbind_config(struct usb_configuration *c)
{
  personality_acm_ser_ser_opt_unbind_config(c, SKIP_SER2);
}

static int  personality_acm_ser_ser_bind_config(struct usb_configuration *c)
{
  return personality_acm_ser_ser_opt_bind_config(c, EVERYTHING);
}
static void personality_acm_ser_ser_unbind_config(struct usb_configuration *c)
{
  personality_acm_ser_ser_opt_unbind_config(c, EVERYTHING);
}

static int  personality_acm_ser_ser_msc_bind_config(struct usb_configuration *c) // 0x618e without adb
{
  int result = personality_acm_ser_ser_bind_config(c);
  if (0 == result) {
    result = personality_msc_bind_config(c);
    if (0 == result) {
      return 0;
    }
    personality_acm_ser_ser_unbind_config(c);
  }
  return result;
}
static void personality_acm_ser_ser_msc_unbind_config(struct usb_configuration *c) // 0x618e without adb
{
  personality_msc_unbind_config(c);
  personality_acm_ser_ser_unbind_config(c);
}

static int  personality_acm_ser_ser_msc_adb_bind_config(struct usb_configuration *c) // 0x618e with adb
{
  int result = personality_acm_ser_ser_msc_bind_config(c);
  if (0 == result) {
    result = personality_adb_bind_config(c);
    if (0 == result) {
      return 0;
    }
    personality_acm_ser_ser_msc_unbind_config(c);
  }
  return result;
}
static void personality_acm_ser_ser_msc_adb_unbind_config(struct usb_configuration *c) // 0x618e with adb
{
  personality_adb_unbind_config(c);
  personality_acm_ser_ser_msc_unbind_config(c);
}

void personality_acm_ser(struct usb_composite_dev* cdev, const char* name) // 0x61a5
{
  static struct usb_configuration config = {
    .bind = personality_acm_ser_bind_config,
    .unbind = personality_acm_ser_unbind_config,
  };
  personality_usb_add_config(cdev, name, &config);
}
void personality_acm_ser_ser_msc(struct usb_composite_dev* cdev, const char* name) // 0x61a4
{
  static struct usb_configuration config = {
    .bind = personality_acm_ser_ser_msc_bind_config,
    .unbind = personality_acm_ser_ser_msc_unbind_config,
  };
  personality_usb_add_config(cdev, name, &config);
}
void personality_acm_ser_ser_msc_adb(struct usb_composite_dev* cdev, const char* name) // 0x61a3
{
  static struct usb_configuration config = {
    .bind = personality_acm_ser_ser_msc_adb_bind_config,
    .unbind = personality_acm_ser_ser_msc_adb_unbind_config,
  };
  personality_usb_add_config(cdev, name, &config);
}
void personality_ser_ecm_acm_ser_msc(struct usb_composite_dev* cdev, const char* name) // 0x61a2
{
  static struct usb_configuration config = {
    .bind = personality_ser_ecm_acm_ser_msc_bind_config,
    .unbind = personality_ser_ecm_acm_ser_msc_unbind_config,
  };
  personality_usb_add_config(cdev, name, &config);
}
void personality_ser_ecm_acm_ser_msc_adb(struct usb_composite_dev* cdev, const char* name) // 0x61a1
{
  static struct usb_configuration config = {
    .bind = personality_ser_ecm_acm_ser_msc_adb_bind_config,
    .unbind = personality_ser_ecm_acm_ser_msc_adb_unbind_config,
  };
  personality_usb_add_config(cdev, name, &config);
}
