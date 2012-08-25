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

#ifndef DRIVERS_USB_GADGET_PERSONALITY_PECMMSC_H
#define DRIVERS_USB_GADGET_PERSONALITY_PECMMSC_H

struct usb_composite_dev;

void add_ecm_msc_configurations(struct usb_composite_dev* cdev, const char* name);
void add_adb_ecm_msc_configurations(struct usb_composite_dev* cdev, const char* name);

#endif /* ndef DRIVERS_USB_GADGET_PERSONALITY_PECMMSC_H */
