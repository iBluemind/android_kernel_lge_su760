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

#ifndef DRIVERS_USB_GADGET_PERSONALITY_PSERIAL_H
#define DRIVERS_USB_GADGET_PERSONALITY_PSERIAL_H

struct usb_configuration;

int personality_acm_bind_config(struct usb_configuration *c, int port_num);
void personality_acm_unbind_config(struct usb_configuration *c, int port_num);

int personality_ser_bind_config(struct usb_configuration *c, int port_num);
void personality_ser_unbind_config(struct usb_configuration *c, int port_num);

#endif /* ndef DRIVERS_USB_GADGET_PERSONALITY_PSERIAL_H */
