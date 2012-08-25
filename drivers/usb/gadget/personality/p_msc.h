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

#ifndef DRIVERS_USB_GADGET_PERSONALITY_PMSC_H
#define DRIVERS_USB_GADGET_PERSONALITY_PMSC_H

struct usb_composite_dev;
struct usb_configuration;

int personality_msc_bind_config(struct usb_configuration *c);
void personality_msc_unbind_config(struct usb_configuration *c);

void personality_msc(struct usb_composite_dev* cdev, const char* name);

#endif /* ndef DRIVERS_USB_GADGET_PERSONALITY_PMSC_H */
