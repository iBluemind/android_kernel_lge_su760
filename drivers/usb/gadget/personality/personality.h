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
#ifndef DRIVERS_USB_GADGET_PERSONALITY_PERSONALITY_H
#define DRIVERS_USB_GADGET_PERSONALITY_PERSONALITY_H

enum {
  /* Selected usb device mode values */
  USB_CDROM_FOR_PCSUITE = 31,
  USB_CDROM_FOR_CP = 32,
  /* StringDescriptorIds */
  USB_PERSONALITY_OS_STRING_DESCRIPTOR = 0xee,
  USB_PERSONALITY_MANUFACTURER = 0x51,
  USB_PERSONALITY_PRODUCT,
  USB_PERSONALITY_SERIAL_NUMBER,
  USB_PERSONALITY_INTERFACE_ADB,
  USB_PERSONALITY_INTERFACE_CDC,
  USB_PERSONALITY_INTERFACE_DPS,
  USB_PERSONALITY_INTERFACE_MTP,
  /* RequestIds */
  USB_PERSONALITY_GET_MICROSOFT_OS_FEATURE_DESCRIPTOR_REQUEST = 0xFE,
  /* This number is also hardcoded in the MSFT string. */

  /* Android user- and group-id for the "system". */
  AID_SYSTEM = 1000,
};

struct kset * personality_cdev_to_kset(struct usb_composite_dev* cdev);
struct usb_device_descriptor* personality_cdev_to_device_desc(struct usb_composite_dev* cdev);

/**
 * Returns the id of the currently selected USB personality.

 * @param cdev pointer to a composite device representing a
 *             personality gadget.
 * @return personality id
 */
/* int get_personality(struct usb_composite_dev* cdev); */

/**
 * Changes the next personality. The device will re-enumerate to
 * this personality immediately (if soft-reconnect is supported)
 * or at the next connect (initiated by the user).
 * Note: The actual switch is deferred and doesn't happen in the
 * context of the caller, because that may take some time, so this
 * may be called from any context.

 * @param cdev pointer to a composite device representing a
 *             personality gadget.
 * @param next personality id
 * @return zero or negative error code
 */
int set_next_personality(struct usb_composite_dev* cdev, int next);

/** Initializes additional @c config fields and calls usb_add_config() */
int personality_usb_add_config(struct usb_composite_dev* cdev, const char* name,
                               struct usb_configuration* config);

#endif /* ndef DRIVERS_USB_GADGET_PERSONALITY_PERSONALITY_H */
