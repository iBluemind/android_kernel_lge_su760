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

#ifndef DRIVERS_USB_GADGET_PERSONALITY_PMSCADB_H
#define DRIVERS_USB_GADGET_PERSONALITY_PMSCADB_H

#include "p_adb.h"
#include "p_msc.h"

void personality_msc_adb(struct usb_composite_dev* cdev, const char* name);

#endif /* ndef DRIVERS_USB_GADGET_PERSONALITY_PMSCADB_H */
