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

#include "disable_init_defines.h"
#include "../f_acm.c"
#include "../f_serial.c"

int personality_acm_bind_config(struct usb_configuration *c, int port_num)
{
  return acm_bind_config(c, port_num);
}
void personality_acm_unbind_config(struct usb_configuration *c, int port_num)
{
  /* TODO */
}

int personality_ser_bind_config(struct usb_configuration *c, int port_num)
{
  return gser_bind_config(c, port_num);
}
void personality_ser_unbind_config(struct usb_configuration *c, int port_num)
{
  /* TODO */
}
