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

#ifndef DRIVERS_USB_GADGET_PERSONALITY_PADB_H
#define DRIVERS_USB_GADGET_PERSONALITY_PADB_H

struct usb_composite_dev;
struct usb_configuration;

int personality_adb_bind_config(struct usb_configuration *c);
static inline void personality_adb_unbind_config(struct usb_configuration *c) {}

int personality_adb(struct usb_composite_dev *cdev, struct usb_configuration *c);

#endif /* ndef DRIVERS_USB_GADGET_PERSONALITY_PADB_H */
